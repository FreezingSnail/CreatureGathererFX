#pragma once

#include <cstring>
#include <sstream>

#include "test.hpp"
#include "../tools/battle-sim/BatchRunner.hpp"
#include "../tools/battle-sim/ScenarioBuilder.hpp"
#include "../tools/battle-sim/SwitchPolicy.hpp"
#include "../tools/battle-sim/SimulatorReports.hpp"
#include "../src/engine/battle/BattleSession.hpp"
#include "../src/engine/battle/MoveUses.hpp"
#include "../src/player/Player.hpp"

extern Player player;

#ifdef BATTLE_SIMULATOR
namespace battle_sim_runner_test {

bool sameRecord(const battle_sim::MatchRecord &a,
                const battle_sim::MatchRecord &b)
{
    if (a.ordinal != b.ordinal || a.trial != b.trial || a.seed != b.seed ||
        a.level != b.level ||
        a.scenario != b.scenario || a.policy != b.policy ||
        a.playerCount != b.playerCount || a.opponentCount != b.opponentCount ||
        a.status != b.status || a.turns != b.turns || a.playerHp != b.playerHp ||
        a.opponentHp != b.opponentHp || a.playerDamage != b.playerDamage ||
        a.opponentDamage != b.opponentDamage ||
        a.switchCount[0] != b.switchCount[0] ||
        a.switchCount[1] != b.switchCount[1] ||
        a.moveUses.size() != b.moveUses.size()) {
        return false;
    }
    for (uint8_t slot = 0; slot < 3; ++slot) {
        if (a.playerSpecies[slot] != b.playerSpecies[slot] ||
            a.opponentSpecies[slot] != b.opponentSpecies[slot]) {
            return false;
        }
    }
    for (size_t index = 0; index < a.moveUses.size(); ++index) {
        if (a.moveUses[index].side != b.moveUses[index].side ||
            a.moveUses[index].moveId != b.moveUses[index].moveId ||
            a.moveUses[index].count != b.moveUses[index].count) {
            return false;
        }
    }
    return true;
}

bool sameTrace(const std::vector<battle_sim::TraceEvent> &a,
               const std::vector<battle_sim::TraceEvent> &b)
{
    if (a.size() != b.size()) return false;
    for (size_t index = 0; index < a.size(); ++index) {
        const battle_sim::TraceEvent &left = a[index];
        const battle_sim::TraceEvent &right = b[index];
        if (left.step != right.step || left.kind != right.kind ||
            left.actor != right.actor || left.index != right.index ||
            left.flags != right.flags || left.switchReason != right.switchReason ||
            left.outcome != right.outcome) {
            return false;
        }
        for (uint8_t side = 0; side < 2; ++side) {
            if (left.speciesBefore[side] != right.speciesBefore[side] ||
                left.hpBefore[side] != right.hpBefore[side] ||
                left.hpAfter[side] != right.hpAfter[side]) {
                return false;
            }
        }
    }
    return true;
}

bool makeSwitchScenario(battle_sim::Scenario &scenario)
{
    battle_sim::PartySpec playerParty;
    playerParty.count = 3;
    playerParty.activeSlot = 0;
    for (uint8_t slot = 0; slot < 3; ++slot) {
        playerParty.members[slot].species = slot;
        playerParty.members[slot].level = 10;
        playerParty.members[slot].hp = battle_sim::FULL_HP;
    }
    playerParty.members[2].moveIds[0] = DELUGE_MOVE_ID;

    battle_sim::PartySpec opponentParty;
    opponentParty.count = 1;
    opponentParty.activeSlot = 0;
    opponentParty.members[0].species = 3;
    opponentParty.members[0].level = 10;
    opponentParty.members[0].hp = battle_sim::FULL_HP;

    if (battle_sim::build(playerParty, opponentParty, scenario) !=
        battle_sim::BuildError::None) return false;

    battle::BattleState &state = scenario.state;
    battle::Combatant &active = state.active[0];
    active.types = DualType(Type::WIND, Type::NONE);
    active.hp = 10;
    active.maxHp = active.stats.hp = 100;
    active.stats.speed = 255;
    for (uint8_t move = 0; move < 4; ++move) {
        active.moveIds[move] = 255;
        active.moves[move] = Move();
    }

    battle::Combatant &enemy = state.active[1];
    enemy.hp = 250;
    enemy.maxHp = enemy.stats.hp = 250;
    enemy.stats.attack = 200;
    enemy.stats.speed = 1;
    enemy.types = DualType(Type::WATER, Type::NONE);
    enemy.moveIds[0] = 1;
    enemy.moves[0] = Move(MoveBitSet{
        static_cast<uint8_t>(Type::WATER), 31, 1, 0, 0});

    // Stable original-slot order: slot 1 is a weaker survival upgrade than
    // slot 2 solely because it has less current HP.
    scenario.switchProfiles[0][1] = {
        static_cast<uint8_t>(Type::PLANT), static_cast<uint8_t>(Type::NONE),
        255, 255};
    scenario.switchProfiles[0][2] = scenario.switchProfiles[0][1];
    state.bench[0][0].hp = 20;
    state.bench[0][1].hp = 100;
    return true;
}

void switchTacticalPolicy(TestSuite &suite)
{
    Test test(__func__);
    battle_sim::Scenario scenario{};
    const bool built = makeSwitchScenario(scenario);
    test.assert(built, true,
                "switch policy fixture builds a valid three-member team");
    if (!built) {
        suite.addTest(test);
        return;
    }

    const battle_sim::SwitchDecision survivalDecision =
        battle_sim::chooseSwitchTacticalIntent(scenario, scenario.state, false);
    battle_sim::SwitchDecision decision = survivalDecision;
    test.assert(decision.intent.kind, MenuIntentKind::SelectParty,
                "lethal incoming threat selects a defensive live bench member");
    test.assert(decision.intent.index, static_cast<uint8_t>(2),
                "lowest threat per current HP wins over weaker survival candidate");
    test.assert(decision.reason, battle_sim::SwitchReason::ImprovedSurvival,
                "simulator decision reports the survival reason");

    battle_sim::Scenario tieScenario{};
    makeSwitchScenario(tieScenario);
    tieScenario.state.bench[0][0].hp = 100;
    tieScenario.state.bench[0][1].hp = 100;
    decision = battle_sim::chooseSwitchTacticalIntent(
        tieScenario, tieScenario.state, false);
    test.assert(decision.intent.index, static_cast<uint8_t>(1),
                "equal defensive scores select the lowest original party slot");

    player.basic();
    battle_sim::installPlayerParty(scenario, player);
    scenario.state.bench[0][1].hp = scenario.benchMaxHp[0][1];
    battle::BattleSession session;
    test.assert(session.beginPrepared(scenario.state, scenario.benchMaxHp), true,
                "switch recommendation enters the ordinary shared session");
    StatModifer retained;
    retained.setModifier(StatType::ATTACK_M, 1);
    session.stateForTest().partyModifiers[0][2] = retained.modifiers;
    session.stateForTest().moveUsesSpent[0][2] = 1;
    test.assert(session.submitIntent(survivalDecision.intent), true,
                "BattleSession accepts the policy's SelectParty intent");
    test.assert(session.advance(), true,
                "voluntary switch resolves as the selected action");
    test.assert(session.result().kind, battle::ResultKind::Switch,
                "switch consumes the regular action");
    test.assert(session.state().activeSlot[0], static_cast<uint8_t>(2),
                "policy selects the expected original slot in the engine");
    test.assert(session.state().active[0].statMods.getModifier(StatType::ATTACK_M),
                1, "switching retains the selected creature's stat stages");
    test.assert(battle::remainingMoveUses(session.state(), battle::Side::Player, 0),
                1, "switching retains PP spent on the selected creature");
    test.assert(battle_sim::chooseSwitchTacticalIntent(
                    tieScenario, session.state(), true).intent.kind !=
                    MenuIntentKind::SelectParty, true,
                "repeat-switch guard prevents another immediate voluntary switch");

    battle_sim::Scenario immune{};
    makeSwitchScenario(immune);
    immune.state.active[0].types = DualType(Type::WATER, Type::NONE);
    immune.state.active[1].moves[0] = Move(MoveBitSet{
        static_cast<uint8_t>(Type::FIRE), 31, 1, 0, 0});
    decision = battle_sim::chooseSwitchTacticalIntent(immune, immune.state, false);
    test.assert(decision.intent.kind, MenuIntentKind::Pass,
                "type immunity removes incoming threat and avoids a switch");

    battle_sim::Scenario neutral{};
    makeSwitchScenario(neutral);
    neutral.state.active[0].types = DualType(Type::WATER, Type::NONE);
    neutral.switchProfiles[0][1] = {
        static_cast<uint8_t>(Type::WATER), static_cast<uint8_t>(Type::NONE),
        neutral.state.active[0].stats.defense,
        neutral.state.active[0].stats.spcDef};
    neutral.switchProfiles[0][2] = neutral.switchProfiles[0][1];
    neutral.state.bench[0][0].hp = neutral.state.active[0].hp;
    neutral.state.bench[0][1].hp = neutral.state.active[0].hp;
    decision = battle_sim::chooseSwitchTacticalIntent(neutral, neutral.state, false);
    test.assert(decision.intent.kind, MenuIntentKind::Pass,
                "neutral matchup with no survival gain keeps the fallback action");

    battle_sim::Scenario exhausted{};
    makeSwitchScenario(exhausted);
    exhausted.state.moveUsesSpent[1][0] = 3;
    decision = battle_sim::chooseSwitchTacticalIntent(
        exhausted, exhausted.state, false);
    test.assert(decision.intent.kind, MenuIntentKind::Pass,
                "exhausted opponent move does not create a false threat");

    battle_sim::Scenario noBench{};
    makeSwitchScenario(noBench);
    noBench.state.bench[0][0].hp = 0;
    noBench.state.bench[0][1].hp = 0;
    decision = battle_sim::chooseSwitchTacticalIntent(noBench, noBench.state, false);
    test.assert(decision.intent.kind, MenuIntentKind::Pass,
                "no live bench member preserves the fallback action");

    battle_sim::Scenario oneOnOne{};
    battle_sim::PartySpec solo;
    solo.count = 1;
    solo.members[0].species = 0;
    solo.members[0].level = 10;
    solo.members[0].hp = battle_sim::FULL_HP;
    solo.members[0].moveIds[0] = 8;
    battle_sim::PartySpec soloOpponent = solo;
    soloOpponent.members[0].species = 1;
    battle_sim::build(solo, soloOpponent, oneOnOne);
    decision = battle_sim::chooseSwitchTacticalIntent(
        oneOnOne, oneOnOne.state, false);
    test.assert(decision.intent.kind, MenuIntentKind::SelectMove,
                "one-on-one selection falls back to the existing tactical move");

    battle_sim::Options smoke;
    smoke.mode = battle_sim::Mode::Random3v3;
    smoke.seed = 20261004;
    smoke.level = 10;
    smoke.trials = 8;
    smoke.policy = battle_sim::Policy::SwitchTactical;
    smoke.maxTurns = 100;
    battle_sim::BatchSummary matches;
    std::string error;
    test.assert(battle_sim::runBatch(smoke, matches, error), true,
                "switch-tactical policy runs through the production session");
    uint32_t playerSwitches = 0;
    const battle_sim::MatchRecord *switchedMatch = nullptr;
    bool noTimeouts = true;
    for (const battle_sim::MatchRecord &match : matches.matches) {
        playerSwitches += match.switchCount[0];
        noTimeouts &= match.status != battle_sim::MatchStatus::Timeout;
        if (match.switchCount[0] != 0 && switchedMatch == nullptr)
            switchedMatch = &match;
    }
    test.assert(playerSwitches > 0, true,
                "fixed-seed 3v3 sample exercises voluntary switches");
    test.assert(noTimeouts, true,
                "switch guards prevent loops and all sample battles finish");
    if (switchedMatch != nullptr) {
        battle_sim::ReplayRequest request;
        request.scenario = switchedMatch->scenario;
        request.seed = switchedMatch->seed;
        request.level = smoke.level;
        request.policy = switchedMatch->policy;
        request.maxTurns = smoke.maxTurns;
        for (uint8_t slot = 0; slot < 3; ++slot) {
            request.playerTeam.push_back(switchedMatch->playerSpecies[slot]);
            request.opponentTeam.push_back(switchedMatch->opponentSpecies[slot]);
        }
        battle_sim::ReplayResult replay;
        test.assert(battle_sim::runReplay(request, replay, error), true,
                    "voluntary switch match is reproducible as a replay");
        test.assert(sameRecord(replay.match, *switchedMatch), true,
                    "replay preserves switch count and battle outcome");
        std::ostringstream trace;
        battle_sim::writeTrace(trace, replay);
        test.assert(trace.str().find("improved-survival") != std::string::npos,
                    true, "replay attributes the voluntary switch reason");
    }

    suite.addTest(test);
}

void pairwiseAssignmentsAndReplay(TestSuite &suite)
{
    Test test(__func__);
    battle_sim::Options options;
    options.mode = battle_sim::Mode::Pairwise;
    options.seed = 0x123456789abcdef0ULL;
    options.level = 8;
    options.trials = 1;
    options.policy = battle_sim::Policy::Both;
    options.maxTurns = 20;
    options.speciesCount = 3;

    battle_sim::BatchSummary first;
    battle_sim::BatchSummary repeat;
    std::string error;
    test.assert(battle_sim::runBatch(options, first, error), true,
                "pairwise batch runs through BattleSession");
    test.assert(first.matches.size(), static_cast<size_t>(18),
                "pairwise covers all ordered species pairs and both policies");
    test.assert(battle_sim::runBatch(options, repeat, error), true,
                "same-seed pairwise batch can be replayed");
    bool identical = first.matches.size() == repeat.matches.size();
    for (size_t match = 0; identical && match < first.matches.size(); ++match) {
        identical = sameRecord(first.matches[match], repeat.matches[match]);
    }
    test.assert(identical, true,
                "same options and seed reproduce every match record");

    uint8_t directedPairs[3][3] = {};
    bool greedy = false;
    bool random = false;
    for (const battle_sim::MatchRecord &record : first.matches) {
        test.assert(record.playerCount, static_cast<uint8_t>(1),
                    "pairwise player side has one species");
        test.assert(record.opponentCount, static_cast<uint8_t>(1),
                    "pairwise opponent side has one species");
        ++directedPairs[record.playerSpecies[0]][record.opponentSpecies[0]];
        greedy |= record.policy == battle_sim::Policy::Greedy;
        random |= record.policy == battle_sim::Policy::RandomValidMove;
    }
    for (uint8_t player = 0; player < 3; ++player) {
        for (uint8_t opponent = 0; opponent < 3; ++opponent) {
            test.assert(directedPairs[player][opponent], static_cast<uint8_t>(2),
                        "each ordered pair runs once per labeled policy");
        }
    }
    test.assert(greedy, true, "greedy policy has a distinct result label");
    test.assert(random, true,
                "random-valid-move policy has a distinct result label");
    test.assert(std::strcmp(battle_sim::PRNG_VERSION, "xorshift32-v1") == 0,
                true, "runner exposes its PRNG version");

    if (!first.matches.empty()) {
        const battle_sim::MatchRecord &record = first.matches[0];
        battle_sim::ReplayRequest request;
        request.scenario = record.scenario;
        request.seed = record.seed;
        request.level = options.level;
        request.policy = record.policy;
        request.maxTurns = options.maxTurns;
        battle_sim::ReplayResult replay;
        battle_sim::ReplayResult repeatReplay;
        test.assert(battle_sim::runReplay(request, replay, error), true,
                    "recorded scenario and per-match seed replay directly");
        test.assert(battle_sim::runReplay(request, repeatReplay, error), true,
                    "replay can be repeated without CSV or hidden RNG state");
        test.assert(sameRecord(replay.match, record), true,
                    "replay reproduces the reported match result and metrics");
        test.assert(sameTrace(replay.trace, repeatReplay.trace), true,
                    "replay reproduces its ordered action/result trace");
        std::ostringstream traceOne;
        std::ostringstream traceTwo;
        battle_sim::writeTrace(traceOne, replay);
        battle_sim::writeTrace(traceTwo, repeatReplay);
        test.assert(traceOne.str() == traceTwo.str(), true,
                    "replay trace output is byte-identical");
    }
    suite.addTest(test);
}

void randomThreeVsThreeBalanceAndTimeout(TestSuite &suite)
{
    Test test(__func__);
    battle_sim::Options options;
    options.mode = battle_sim::Mode::Random3v3;
    options.seed = 998877;
    options.level = 11;
    options.trials = 8;
    options.policy = battle_sim::Policy::RandomValidMove;
    options.maxTurns = 1;
    options.speciesCount = 8;

    battle_sim::BatchSummary summary;
    std::string error;
    test.assert(battle_sim::runBatch(options, summary, error), true,
                "random-3v3 batch runs");
    test.assert(summary.matches.size(), static_cast<size_t>(8),
                "one random team pairing is emitted per trial");
    bool balanced = summary.speciesAppearances.size() == 8;
    for (uint8_t species = 0; species < 8 && balanced; ++species) {
        balanced = summary.speciesAppearances[species] == 6;
    }
    test.assert(balanced, true,
                "least-used selection balances appearances across species");

    bool uniqueTeams = true;
    bool capped = true;
    for (const battle_sim::MatchRecord &record : summary.matches) {
        uniqueTeams &= record.playerCount == 3 && record.opponentCount == 3;
        bool seen[8] = {};
        for (uint8_t slot = 0; slot < record.playerCount; ++slot) {
            const uint8_t species = record.playerSpecies[slot];
            if (species >= 8 || seen[species]) uniqueTeams = false;
            else seen[species] = true;
        }
        for (uint8_t slot = 0; slot < record.opponentCount; ++slot) {
            const uint8_t species = record.opponentSpecies[slot];
            if (species >= 8 || seen[species]) uniqueTeams = false;
            else seen[species] = true;
        }
        capped &= record.status == battle_sim::MatchStatus::Timeout &&
                  record.turns == 1;
    }
    test.assert(uniqueTeams, true,
                "random-3v3 selects six unique species in each match");
    test.assert(capped, true,
                "matches at the configured turn cap are labeled timeouts");

    if (!summary.matches.empty()) {
        const battle_sim::MatchRecord &record = summary.matches[0];
        battle_sim::ReplayRequest request;
        request.scenario = record.scenario;
        request.seed = record.seed;
        request.level = options.level;
        request.policy = record.policy;
        request.maxTurns = options.maxTurns;
        for (uint8_t slot = 0; slot < 3; ++slot) {
            request.playerTeam.push_back(record.playerSpecies[slot]);
            request.opponentTeam.push_back(record.opponentSpecies[slot]);
        }
        battle_sim::ReplayResult replay;
        test.assert(battle_sim::runReplay(request, replay, error), true,
                    "random-3v3 report can replay from its roster and seed");
        test.assert(sameRecord(replay.match, record), true,
                    "3v3 replay reproduces outcome, damage, and move counts");
    }

    options.speciesCount = 5;
    test.assert(battle_sim::runBatch(options, summary, error), false,
                "random-3v3 rejects fewer than six configured species");
    suite.addTest(test);
}

void namedPresetAnchors(TestSuite &suite)
{
    Test test(__func__);
    battle_sim::Options options;
    options.mode = battle_sim::Mode::Anchors;
    options.seed = 4;
    options.policy = battle_sim::Policy::Both;
    options.maxTurns = 3;
    battle_sim::BatchSummary summary;
    std::string error;
    test.assert(battle_sim::runBatch(options, summary, error), true,
                "named preset anchors run headlessly");
    test.assert(summary.matches.size(), static_cast<size_t>(4),
                "both anchors run once per policy");
    bool opening = false;
    bool drill = false;
    for (const battle_sim::MatchRecord &record : summary.matches) {
        opening |= record.scenario == "opening";
        drill |= record.scenario == "switch_drill";
    }
    test.assert(opening, true, "opening is a named anchor scenario");
    test.assert(drill, true, "switch_drill is a named anchor scenario");
    if (!summary.matches.empty()) {
        const battle_sim::MatchRecord &record = summary.matches[0];
        battle_sim::ReplayRequest request;
        request.scenario = record.scenario;
        request.seed = record.seed;
        request.level = options.level;
        request.policy = record.policy;
        request.maxTurns = options.maxTurns;
        battle_sim::ReplayResult replay;
        test.assert(battle_sim::runReplay(request, replay, error), true,
                    "named preset replay rebuilds from generated data");
        test.assert(sameRecord(replay.match, record), true,
                    "preset replay reproduces match summary");
    }
    suite.addTest(test);
}

void expandedRosterBoundaries(TestSuite &suite)
{
    Test test(__func__);
    battle_sim::Options options;
    options.mode = battle_sim::Mode::Pairwise;
    options.speciesCount = 64;
    options.seed = 20261007;
    options.trials = 1;
    options.policy = battle_sim::Policy::Greedy;
    options.maxTurns = 40;
    battle_sim::BatchSummary summary;
    std::string error;
    test.assert(battle_sim::runBatch(options, summary, error), true,
                "pairwise runner accepts all 64 canonical species");
    test.assert(summary.matches.size(), static_cast<size_t>(64 * 64),
                "expanded pairwise matrix covers every ordered matchup");
    test.assert(summary.matches.back().scenario == "pair_63_63", true,
                "expanded matrix reaches boundary species 63");

    battle_sim::ReplayRequest request;
    request.scenario = "pair_63_63";
    request.seed = 99;
    request.policy = battle_sim::Policy::Greedy;
    battle_sim::ReplayResult replay;
    test.assert(battle_sim::runReplay(request, replay, error), true,
                "boundary pair with species 63 replays through BattleSession");
    test.assert(replay.match.playerSpecies[0], static_cast<uint8_t>(63),
                "replay preserves species 63 in the player roster");
    suite.addTest(test);
}

void stableReportsAndAggregates(TestSuite &suite)
{
    Test test(__func__);
    battle_sim::Options options;
    options.mode = battle_sim::Mode::Pairwise;
    options.seed = 71;
    options.level = 8;
    options.trials = 2;
    options.policy = battle_sim::Policy::Both;
    options.maxTurns = 20;
    options.speciesCount = 2;
    battle_sim::BatchSummary summary;
    std::string error;
    test.assert(battle_sim::runBatch(options, summary, error), true,
                "small report fixture batch runs");

    battle_sim::FixtureProvenance provenance;
    test.assert(battle_sim::readFixtureProvenance(
                    "fxdata/generated/manifest.json", provenance, error),
                true, "generated manifest supplies fixture provenance");
    test.assert(!provenance.creatureSha256.empty() &&
                !provenance.moveSha256.empty() &&
                !provenance.presetSha256.empty(), true,
                "all simulator fixture hashes are present");

    const std::string rawOne = battle_sim::renderMatchesCsv(
        options, summary, provenance);
    const std::string rawTwo = battle_sim::renderMatchesCsv(
        options, summary, provenance);
    const std::string aggregateOne = battle_sim::renderSummaryCsv(
        options, summary, provenance);
    const std::string aggregateTwo = battle_sim::renderSummaryCsv(
        options, summary, provenance);
    test.assert(rawOne == rawTwo && aggregateOne == aggregateTwo, true,
                "raw and summary CSV are byte-stable for the same batch");
    test.assert(rawOne.find("# report_schema_version,2\n") !=
                std::string::npos, true, "raw CSV has a versioned schema");
    test.assert(rawOne.find("# fixture_sha256,creature_data.hpp,") !=
                std::string::npos, true,
                "raw CSV identifies generated fixture provenance");
    test.assert(rawOne.find("player_damage,opponent_damage,") !=
                std::string::npos, true,
                "raw CSV includes damage totals and move counts");
    const size_t rawFirst = rawOne.find("1,pair_00_00,1,greedy,");
    const size_t rawSecond = rawOne.find("2,pair_00_00,1,random-valid-move,");
    const size_t rawThird = rawOne.find("3,pair_00_00,2,greedy,");
    test.assert(rawFirst < rawSecond && rawSecond < rawThird, true,
                "raw rows keep stable scenario, trial, policy order");
    test.assert(aggregateOne.find(
                    "scenario,policy,player_team,opponent_team,matches,") !=
                std::string::npos, true,
                "aggregate CSV preserves both seat assignments");
    test.assert(aggregateOne.find("pair_00_00,greedy,0,0,2,") !=
                std::string::npos, true,
                "summary aggregates expected pair and trial counts");
    test.assert(aggregateOne.find("pair_00_00,greedy,0,0,2,") <
                aggregateOne.find("pair_00_00,random-valid-move,0,0,2,"),
                true, "summary groups have stable policy ordering");
    test.assert(aggregateOne.find("timeouts,") != std::string::npos &&
                aggregateOne.find("player_move_use_counts,") !=
                std::string::npos &&
                aggregateOne.find("player_switches,opponent_switches") !=
                std::string::npos, true,
                "summary preserves timeouts, move-use totals, and switches");

    suite.addTest(test);
}

} // namespace battle_sim_runner_test

inline void BattleSimulatorRunnerSuite(TestRunner &runner)
{
    TestSuite suite("Battle simulator batch runner");
    battle_sim_runner_test::switchTacticalPolicy(suite);
    battle_sim_runner_test::pairwiseAssignmentsAndReplay(suite);
    battle_sim_runner_test::randomThreeVsThreeBalanceAndTimeout(suite);
    battle_sim_runner_test::namedPresetAnchors(suite);
    battle_sim_runner_test::expandedRosterBoundaries(suite);
    battle_sim_runner_test::stableReportsAndAggregates(suite);
    runner.addTestSuite(suite);
}
#endif
