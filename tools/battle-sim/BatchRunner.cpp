#include "BatchRunner.hpp"

#if defined(BATTLE_SIMULATOR) && !defined(__AVR__)

#include <algorithm>
#include <array>
#include <charconv>
#include <limits>
#include <sstream>
#include <utility>

#include "ScenarioBuilder.hpp"
#include "SwitchPolicy.hpp"
#include "../../src/engine/battle/MoveUses.hpp"
#include "../../src/engine/battle/Ai.hpp"
#include "../../src/engine/battle/BattleSession.hpp"
#include "../../src/engine/battle/Resolve.hpp"
#include "../../src/player/Player.hpp"

extern Player player;

namespace battle_sim {
namespace {

constexpr uint64_t SEED_STEP = 0x9e3779b97f4a7c15ULL;
constexpr uint32_t ZERO_STATE_REPLACEMENT = 0x6d2b79f5UL;
constexpr uint64_t MAX_BATCH_MATCHES = 100000;

uint32_t battleRngState = ZERO_STATE_REPLACEMENT;

uint32_t next32(uint32_t &state)
{
    uint32_t value = state;
    value ^= value << 13;
    value ^= value >> 17;
    value ^= value << 5;
    state = value;
    return value;
}

uint32_t bounded(uint32_t &state, uint32_t bound)
{
    if (bound == 0) return 0;
    const uint32_t threshold = static_cast<uint32_t>(-bound) % bound;
    for (;;) {
        const uint32_t value = next32(state);
        if (value >= threshold) return value % bound;
    }
}

uint8_t battleRoll(uint8_t bound)
{
    return static_cast<uint8_t>(bounded(battleRngState, bound));
}

uint64_t mix64(uint64_t value)
{
    value += SEED_STEP;
    value = (value ^ (value >> 30)) * 0xbf58476d1ce4e5b9ULL;
    value = (value ^ (value >> 27)) * 0x94d049bb133111ebULL;
    return value ^ (value >> 31);
}

uint64_t matchSeed(uint64_t seed, uint32_t scenarioOrdinal, Policy policy)
{
    const uint64_t policyKey = policy == Policy::Greedy ? 0x475245454459ULL
        : policy == Policy::Tactical ? 0x544143544943ULL
        : policy == Policy::SwitchTactical ? 0x535749544348ULL
        : 0x52414e444f4dULL;
    return mix64(seed ^ mix64(static_cast<uint64_t>(scenarioOrdinal)) ^ policyKey);
}

void resetBattleRng(uint64_t seed)
{
    battleRngState = static_cast<uint32_t>(seed ^ (seed >> 32));
    if (battleRngState == 0) battleRngState = ZERO_STATE_REPLACEMENT;
}

uint8_t originalSpecies(const battle::BattleState &state, uint8_t side,
                        uint8_t originalSlot)
{
    if (originalSlot == state.activeSlot[side]) return state.active[side].id;
    uint8_t bench = 0;
    for (uint8_t slot = 0; slot < originalSlot; ++slot) {
        if (slot != state.activeSlot[side]) ++bench;
    }
    return state.bench[side][bench].id;
}

void captureTeamIds(const Scenario &scenario, MatchRecord &record)
{
    record.playerCount = scenario.state.partyCount[0];
    record.opponentCount = scenario.state.partyCount[1];
    for (uint8_t slot = 0; slot < record.playerCount; ++slot) {
        record.playerSpecies[slot] = originalSpecies(scenario.state, 0, slot);
    }
    for (uint8_t slot = 0; slot < record.opponentCount; ++slot) {
        record.opponentSpecies[slot] = originalSpecies(scenario.state, 1, slot);
    }
}

uint8_t randomMoveSlot(const battle::BattleState &state)
{
    const battle::Combatant &active = state.active[0];
    uint8_t legal[4];
    uint8_t count = 0;
    for (uint8_t slot = 0; slot < 4; ++slot) {
        if (active.moveIds[slot] != 255 &&
            battle::remainingMoveUses(state, battle::Side::Player, slot) != 0)
            legal[count++] = slot;
    }
    if (count == 0) return 255;
    return legal[bounded(battleRngState, count)];
}

bool choosePlayerMove(const battle::BattleState &state, Policy policy,
                      uint8_t &slot, std::string &error)
{
    if (policy == Policy::Greedy || policy == Policy::Tactical ||
        policy == Policy::SwitchTactical) {
        const battle::BattleAction action =
            policy == Policy::Tactical
                ? battle::chooseAction(state, battle::Side::Player)
                : battle::chooseDamageAction(state, battle::Side::Player);
        if (action.kind != battle::ActionKind::Attack || action.index >= 4 ||
            state.active[0].moveIds[action.index] == 255) {
            slot = 255;
            return true;
        }
        slot = action.index;
        return true;
    }

    slot = randomMoveSlot(state);
    if (slot == 255) {
        return true;
    }
    return true;
}

void observeResult(const battle::ActionResult &result, MatchRecord &record,
                   std::vector<TraceEvent> *trace,
                   SwitchReason switchReason, bool &playerSwitchLocked)
{
    if (result.kind == battle::ResultKind::Switch &&
        static_cast<uint8_t>(result.actor) < 2) {
        ++record.switchCount[static_cast<uint8_t>(result.actor)];
        if (result.actor == battle::Side::Player &&
            (result.flags & battle::FORCED_SWITCH) == 0) {
            playerSwitchLocked = true;
        }
    } else if (result.actor == battle::Side::Player &&
               result.kind != battle::ResultKind::None &&
               result.kind != battle::ResultKind::EndTurn) {
        // Any executed non-switch action permits a later voluntary switch.
        playerSwitchLocked = false;
    }
    if (result.kind == battle::ResultKind::Attack &&
        static_cast<uint8_t>(result.actor) < 2 && result.index != 255) {
        const uint8_t actor = static_cast<uint8_t>(result.actor);
        MoveUse *use = nullptr;
        for (MoveUse &candidate : record.moveUses) {
            if (candidate.side == actor && candidate.moveId == result.index) {
                use = &candidate;
                break;
            }
        }
        if (use == nullptr) {
            record.moveUses.push_back({actor, result.index, 0});
            use = &record.moveUses.back();
        }
        ++use->count;

        if ((result.flags & battle::SELF_HIT) == 0) {
            const uint8_t target = static_cast<uint8_t>(1 - actor);
            const uint8_t hpLost = result.hpBefore[target] > result.hpAfter[target]
                ? static_cast<uint8_t>(result.hpBefore[target] -
                                       result.hpAfter[target])
                : 0;
            if (actor == 0) record.playerDamage += hpLost;
            else record.opponentDamage += hpLost;
        }
    }

    if (trace != nullptr) {
        TraceEvent event;
        event.step = static_cast<uint32_t>(trace->size() + 1);
        event.kind = static_cast<uint8_t>(result.kind);
        event.actor = static_cast<uint8_t>(result.actor);
        event.index = result.index;
        event.flags = result.flags;
        event.switchReason = result.kind == battle::ResultKind::Switch
            ? ((result.flags & battle::FORCED_SWITCH) != 0
                ? static_cast<uint8_t>(SwitchReason::ForcedReplacement)
                : static_cast<uint8_t>(switchReason))
            : static_cast<uint8_t>(SwitchReason::None);
        event.outcome = static_cast<uint8_t>(result.outcome);
        for (uint8_t side = 0; side < 2; ++side) {
            event.speciesBefore[side] = result.speciesBefore[side];
            event.hpBefore[side] = result.hpBefore[side];
            event.hpAfter[side] = result.hpAfter[side];
        }
        trace->push_back(event);
    }
}

bool driveMatch(const Scenario &scenario, Policy policy, uint64_t seed,
                uint16_t maxTurns, MatchRecord &record, std::string &error,
                std::vector<TraceEvent> *trace = nullptr)
{
    installPlayerParty(scenario, player);
    battle::BattleSession session;
    resetBattleRng(seed);
    session.setRng({battleRoll});
    if (!session.beginPrepared(scenario.state, scenario.benchMaxHp)) {
        error = "BattleSession rejected a scenario from the generated fixtures";
        return false;
    }

    uint16_t completedTurns = 0;
    bool playerSwitchLocked = false;
    SwitchReason pendingSwitchReason = SwitchReason::None;
    const uint32_t actionLimit = static_cast<uint32_t>(maxTurns) * 16u + 32u;
    for (uint32_t actions = 0; actions < actionLimit; ++actions) {
        if (session.awaitingPlayer()) {
            MenuIntent intent{};
            if (session.awaitingReplacement()) {
                const battle::PartySnapshot choices = session.partyChoices();
                if (choices.count == 0) {
                    error = "forced replacement requested with no live bench member";
                    return false;
                }
                // partyChoices preserves original slot order; select the first
                // live original slot for a deterministic forced replacement.
                intent = {MenuIntentKind::SelectParty, choices.choices[0].slot};
            } else {
                if (policy == Policy::SwitchTactical) {
                    const SwitchDecision decision = chooseSwitchTacticalIntent(
                        scenario, session.state(), playerSwitchLocked);
                    pendingSwitchReason = decision.reason;
                    intent = decision.intent;
                } else {
                uint8_t moveSlot = 0;
                if (!choosePlayerMove(session.state(), policy, moveSlot, error)) {
                    return false;
                }
                intent = {moveSlot == 255 ? MenuIntentKind::Pass : MenuIntentKind::SelectMove, moveSlot};
                }
            }
            if (!session.submitIntent(intent)) {
                error = "BattleSession refused a simulator player intent";
                return false;
            }
        }

        if (!session.advance()) {
            error = "BattleSession could not advance from its current phase";
            return false;
        }

        const battle::ActionResult result = session.result();
        observeResult(result, record, trace, pendingSwitchReason,
                      playerSwitchLocked);
        if (result.actor == battle::Side::Player &&
            result.kind == battle::ResultKind::Switch) {
            pendingSwitchReason = SwitchReason::None;
        }
        if (result.kind == battle::ResultKind::EndTurn) {
            ++completedTurns;
            record.turns = completedTurns;
        }
        if (result.outcome != battle::Outcome::None) {
            if (result.kind != battle::ResultKind::EndTurn) {
                record.turns = static_cast<uint16_t>(completedTurns + 1u);
            }
            if (result.outcome == battle::Outcome::Win) {
                record.status = MatchStatus::Win;
            } else if (result.outcome == battle::Outcome::Lose) {
                record.status = MatchStatus::Loss;
            } else {
                error = "balance matches produced a non-battle terminal outcome";
                return false;
            }
            session.finishPresentation();
            const battle::BattleView view = session.view();
            for (uint8_t slot = 0; slot < view.partyCount[0]; ++slot) {
                record.playerHp += view.party[0][slot].hp;
            }
            for (uint8_t slot = 0; slot < view.partyCount[1]; ++slot) {
                record.opponentHp += view.party[1][slot].hp;
            }
            return true;
        }

        session.finishPresentation();
        if (result.kind == battle::ResultKind::EndTurn &&
            completedTurns >= maxTurns) {
            record.status = MatchStatus::Timeout;
            record.turns = maxTurns;
            const battle::BattleView view = session.view();
            for (uint8_t slot = 0; slot < view.partyCount[0]; ++slot) {
                record.playerHp += view.party[0][slot].hp;
            }
            for (uint8_t slot = 0; slot < view.partyCount[1]; ++slot) {
                record.opponentHp += view.party[1][slot].hp;
            }
            return true;
        }
    }

    error = "BattleSession exceeded its bounded action count";
    return false;
}

void appendRecord(const Options &options, const Scenario &scenario,
                  const std::string &name, uint16_t trial,
                  uint32_t scenarioOrdinal, Policy policy,
                  uint32_t &nextOrdinal, BatchSummary &summary,
                  std::string &error)
{
    MatchRecord record;
    record.ordinal = nextOrdinal++;
    record.trial = trial;
    record.seed = matchSeed(options.seed, scenarioOrdinal, policy);
    record.level = options.level;
    record.scenario = name;
    record.policy = policy;
    captureTeamIds(scenario, record);
    if (!driveMatch(scenario, policy, record.seed, options.maxTurns,
                    record, error)) {
        return;
    }
    summary.matches.push_back(std::move(record));
}

void addAppearances(BatchSummary &summary, const PartySpec &playerParty,
                    const PartySpec &opponentParty)
{
    for (uint8_t slot = 0; slot < playerParty.count; ++slot) {
        ++summary.speciesAppearances[playerParty.members[slot].species];
    }
    for (uint8_t slot = 0; slot < opponentParty.count; ++slot) {
        ++summary.speciesAppearances[opponentParty.members[slot].species];
    }
}

void chooseBalancedSix(uint8_t speciesCount, uint32_t &selectionState,
                       BatchSummary &summary, uint8_t (&selected)[6])
{
    bool used[32] = {};
    for (uint8_t pick = 0; pick < 6; ++pick) {
        uint32_t minimum = std::numeric_limits<uint32_t>::max();
        for (uint8_t species = 0; species < speciesCount; ++species) {
            if (!used[species]) {
                minimum = std::min(minimum,
                    summary.speciesAppearances[species]);
            }
        }
        uint8_t candidates[32];
        uint8_t count = 0;
        for (uint8_t species = 0; species < speciesCount; ++species) {
            if (!used[species] &&
                summary.speciesAppearances[species] == minimum) {
                candidates[count++] = species;
            }
        }
        const uint8_t chosen = candidates[bounded(selectionState, count)];
        used[chosen] = true;
        selected[pick] = chosen;
        ++summary.speciesAppearances[chosen];
    }

    // Fisher-Yates randomizes which of the balanced selections receives each
    // team slot while keeping each species unique across both teams.
    for (uint8_t i = 5; i > 0; --i) {
        const uint8_t j = static_cast<uint8_t>(bounded(selectionState, i + 1));
        std::swap(selected[i], selected[j]);
    }
}

std::string pairName(uint8_t playerSpecies, uint8_t opponentSpecies)
{
    std::ostringstream out;
    out << "pair_";
    if (playerSpecies < 10) out << '0';
    out << static_cast<unsigned>(playerSpecies) << '_';
    if (opponentSpecies < 10) out << '0';
    out << static_cast<unsigned>(opponentSpecies);
    return out.str();
}

uint64_t expectedMatchCount(const Options &options)
{
    uint64_t scenarios = 0;
    switch (options.mode) {
    case Mode::Pairwise:
        scenarios = static_cast<uint64_t>(options.speciesCount) *
                    options.speciesCount * options.trials;
        break;
    case Mode::Random3v3:
        scenarios = options.trials;
        break;
    case Mode::Anchors:
        scenarios = static_cast<uint64_t>(2) * options.trials;
        break;
    }
    const uint8_t policyCount = options.policy == Policy::Both ? 2 : 1;
    return scenarios * policyCount;
}

} // namespace

const char *modeName(Mode mode)
{
    switch (mode) {
    case Mode::Pairwise: return "pairwise";
    case Mode::Random3v3: return "random-3v3";
    case Mode::Anchors: return "anchors";
    }
    return "unknown";
}

const char *policyName(Policy policy)
{
    switch (policy) {
    case Policy::Greedy: return "greedy";
    case Policy::RandomValidMove: return "random-valid-move";
    case Policy::Both: return "both";
    case Policy::Tactical: return "tactical";
    case Policy::SwitchTactical: return "switch-tactical";
    }
    return "unknown";
}

const char *statusName(MatchStatus status)
{
    switch (status) {
    case MatchStatus::Win: return "win";
    case MatchStatus::Loss: return "loss";
    case MatchStatus::Timeout: return "timeout";
    }
    return "unknown";
}

bool runBatch(const Options &options, BatchSummary &summary,
              std::string &error)
{
    error.clear();
    if (options.level == 0 || options.level > 31) {
        error = "level must be in [1, 31]";
        return false;
    }
    if (options.trials == 0 || options.maxTurns == 0) {
        error = "trials and maximum turns must be nonzero";
        return false;
    }
    if (options.speciesCount == 0 || options.speciesCount > 32) {
        error = "species count must be in [1, 32]";
        return false;
    }
    if (options.mode != Mode::Pairwise && options.mode != Mode::Random3v3 &&
        options.mode != Mode::Anchors) {
        error = "mode must be pairwise, random-3v3, or anchors";
        return false;
    }
    if (options.policy != Policy::Greedy &&
        options.policy != Policy::RandomValidMove &&
        options.policy != Policy::Both && options.policy != Policy::Tactical &&
        options.policy != Policy::SwitchTactical) {
        error = "policy must be greedy, random-valid-move, tactical, switch-tactical, or both";
        return false;
    }
    if (options.mode == Mode::Random3v3 && options.speciesCount < 6) {
        error = "random-3v3 requires at least six configured species";
        return false;
    }
    if (expectedMatchCount(options) > MAX_BATCH_MATCHES) {
        error = "requested batch exceeds the 100000 match safety limit";
        return false;
    }

    BatchSummary next;
    next.speciesAppearances.assign(options.speciesCount, 0);
    const Policy policies[2] = {options.policy == Policy::Both ? Policy::Greedy : options.policy, Policy::RandomValidMove};
    const uint8_t policyCount = options.policy == Policy::Both ? 2 : 1;
    uint32_t nextOrdinal = 1;
    uint32_t scenarioOrdinal = 0;
    uint32_t selectionState = static_cast<uint32_t>(
        mix64(options.seed ^ 0x52414e444f4d3356ULL));
    if (selectionState == 0) selectionState = ZERO_STATE_REPLACEMENT;

    if (options.mode == Mode::Pairwise) {
        for (uint8_t playerSpecies = 0;
             playerSpecies < options.speciesCount; ++playerSpecies) {
            for (uint8_t opponentSpecies = 0;
                 opponentSpecies < options.speciesCount; ++opponentSpecies) {
                for (uint32_t trialIndex = 1; trialIndex <= options.trials;
                     ++trialIndex) {
                    PartySpec playerParty;
                    playerParty.count = 1;
                    playerParty.members[0] = speciesMember(playerSpecies,
                                                           options.level);
                    PartySpec opponentParty;
                    opponentParty.count = 1;
                    opponentParty.members[0] = speciesMember(opponentSpecies,
                                                             options.level);
                    Scenario scenario{};
                    const BuildError buildError =
                        build(playerParty, opponentParty, scenario);
                    if (buildError != BuildError::None) {
                        error = "failed to construct pairwise fixture scenario";
                        return false;
                    }
                    addAppearances(next, playerParty, opponentParty);
                    for (uint8_t p = 0; p < policyCount; ++p) {
                        appendRecord(options, scenario,
                                     pairName(playerSpecies, opponentSpecies),
                                     static_cast<uint16_t>(trialIndex),
                                     scenarioOrdinal, policies[p],
                                     nextOrdinal, next, error);
                        if (!error.empty()) return false;
                    }
                    ++scenarioOrdinal;
                }
            }
        }
    } else if (options.mode == Mode::Random3v3) {
        for (uint32_t trialIndex = 1; trialIndex <= options.trials;
             ++trialIndex) {
            uint8_t selected[6];
            chooseBalancedSix(options.speciesCount, selectionState, next,
                              selected);
            PartySpec playerParty;
            playerParty.count = 3;
            PartySpec opponentParty;
            opponentParty.count = 3;
            for (uint8_t slot = 0; slot < 3; ++slot) {
                playerParty.members[slot] = speciesMember(selected[slot],
                                                         options.level);
                opponentParty.members[slot] = speciesMember(selected[slot + 3],
                                                            options.level);
            }
            Scenario scenario{};
            if (build(playerParty, opponentParty, scenario) != BuildError::None) {
                error = "failed to construct random-3v3 fixture scenario";
                return false;
            }
            for (uint8_t p = 0; p < policyCount; ++p) {
                appendRecord(options, scenario, "random_3v3",
                             static_cast<uint16_t>(trialIndex),
                             scenarioOrdinal, policies[p], nextOrdinal,
                             next, error);
                if (!error.empty()) return false;
            }
            ++scenarioOrdinal;
        }
    } else {
        const BattlePresets::Preset *presets[2] = {
            &BattlePresets::opening, &BattlePresets::switch_drill,
        };
        const char *names[2] = {"opening", "switch_drill"};
        for (uint8_t anchor = 0; anchor < 2; ++anchor) {
            for (uint32_t trialIndex = 1; trialIndex <= options.trials;
                 ++trialIndex) {
                Scenario scenario{};
                if (buildPreset(*presets[anchor], scenario) != BuildError::None) {
                    error = "failed to construct generated trainer preset";
                    return false;
                }
                for (uint8_t p = 0; p < policyCount; ++p) {
                    appendRecord(options, scenario, names[anchor],
                                 static_cast<uint16_t>(trialIndex),
                                 scenarioOrdinal, policies[p], nextOrdinal,
                                 next, error);
                    if (!error.empty()) return false;
                }
                ++scenarioOrdinal;
            }
        }
    }

    summary = std::move(next);
    return true;
}

namespace {

bool parsePairName(const std::string &name, uint8_t &playerSpecies,
                   uint8_t &opponentSpecies)
{
    if (name.compare(0, 5, "pair_") != 0) return false;
    const size_t separator = name.find('_', 5);
    if (separator == std::string::npos) return false;
    unsigned player = 0;
    unsigned opponent = 0;
    const char *beginPlayer = name.data() + 5;
    const char *endPlayer = name.data() + separator;
    const char *beginOpponent = name.data() + separator + 1;
    const char *endOpponent = name.data() + name.size();
    const auto parsedPlayer = std::from_chars(beginPlayer, endPlayer, player);
    const auto parsedOpponent = std::from_chars(beginOpponent, endOpponent,
                                                opponent);
    if (parsedPlayer.ec != std::errc{} || parsedPlayer.ptr != endPlayer ||
        parsedOpponent.ec != std::errc{} || parsedOpponent.ptr != endOpponent ||
        player >= 32 || opponent >= 32) {
        return false;
    }
    playerSpecies = static_cast<uint8_t>(player);
    opponentSpecies = static_cast<uint8_t>(opponent);
    return true;
}

PartySpec replayParty(const std::vector<uint8_t> &team, uint8_t level)
{
    PartySpec party;
    party.count = static_cast<uint8_t>(team.size());
    party.activeSlot = 0;
    for (uint8_t slot = 0; slot < party.count; ++slot) {
        party.members[slot] = speciesMember(team[slot], level);
    }
    return party;
}

bool buildReplayScenario(const ReplayRequest &request, Scenario &scenario,
                         std::string &error)
{
    if (request.scenario == "opening" ||
        request.scenario == "switch_drill" || request.scenario == "utility") {
        const BattlePresets::Preset &preset =
            request.scenario == "opening" ? BattlePresets::opening
                                           : request.scenario == "utility" ? BattlePresets::utility
                                                                           : BattlePresets::switch_drill;
        if (buildPreset(preset, scenario) != BuildError::None) {
            error = "failed to reconstruct the named anchor scenario";
            return false;
        }
        return true;
    }

    if (request.scenario == "random_3v3") {
        if (request.playerTeam.size() != 3 ||
            request.opponentTeam.size() != 3) {
            error = "random_3v3 replay needs three player and opponent species IDs";
            return false;
        }
        const PartySpec playerParty = replayParty(request.playerTeam,
                                                  request.level);
        const PartySpec opponentParty = replayParty(request.opponentTeam,
                                                    request.level);
        if (build(playerParty, opponentParty, scenario) != BuildError::None) {
            error = "random_3v3 replay has an invalid species ID or level";
            return false;
        }
        return true;
    }

    uint8_t playerSpecies = 0;
    uint8_t opponentSpecies = 0;
    if (!parsePairName(request.scenario, playerSpecies, opponentSpecies)) {
        error = "scenario must be pair_NN_NN, random_3v3, opening, or switch_drill";
        return false;
    }
    const std::vector<uint8_t> playerTeam(1, playerSpecies);
    const std::vector<uint8_t> opponentTeam(1, opponentSpecies);
    const PartySpec playerParty = replayParty(playerTeam, request.level);
    const PartySpec opponentParty = replayParty(opponentTeam, request.level);
    if (build(playerParty, opponentParty, scenario) != BuildError::None) {
        error = "pairwise replay has an invalid species ID or level";
        return false;
    }
    return true;
}

} // namespace

bool runReplay(const ReplayRequest &request, ReplayResult &result,
               std::string &error)
{
    error.clear();
    if (request.level == 0 || request.level > 31 || request.maxTurns == 0) {
        error = "replay level must be in [1, 31] and maximum turns must be nonzero";
        return false;
    }
    if (request.policy != Policy::Greedy &&
        request.policy != Policy::RandomValidMove && request.policy != Policy::Tactical &&
        request.policy != Policy::SwitchTactical) {
        error = "replay requires one policy: greedy, random-valid-move, tactical, or switch-tactical";
        return false;
    }

    Scenario scenario{};
    if (!buildReplayScenario(request, scenario, error)) return false;
    ReplayResult next;
    next.match.ordinal = 1;
    next.match.trial = 1;
    next.match.seed = request.seed;
    next.match.level = request.level;
    next.match.scenario = request.scenario;
    next.match.policy = request.policy;
    captureTeamIds(scenario, next.match);
    if (!driveMatch(scenario, request.policy, request.seed, request.maxTurns,
                    next.match, error, &next.trace)) {
        return false;
    }
    result = std::move(next);
    return true;
}

} // namespace battle_sim

#endif
