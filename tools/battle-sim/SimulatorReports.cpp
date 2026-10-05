#include "SimulatorReports.hpp"

#if defined(BATTLE_SIMULATOR) && !defined(__AVR__)

#include <array>
#include <filesystem>
#include <fstream>
#include <map>
#include <sstream>
#include <tuple>

namespace battle_sim {
namespace {

std::string jsonStringField(const std::string &line, const char *field)
{
    const std::string key = std::string("\"") + field + "\"";
    const size_t keyPos = line.find(key);
    if (keyPos == std::string::npos) return {};
    const size_t colon = line.find(':', keyPos + key.size());
    if (colon == std::string::npos) return {};
    const size_t quote = line.find('\"', colon + 1);
    if (quote == std::string::npos) return {};
    const size_t end = line.find('\"', quote + 1);
    if (end == std::string::npos) return {};
    return line.substr(quote + 1, end - quote - 1);
}

void writeMetadata(std::ostream &out, const Options &options,
                   const FixtureProvenance &provenance)
{
    out << "# report_schema_version," << REPORT_SCHEMA_VERSION << '\n'
        << "# simulator_version," << SIMULATOR_VERSION << '\n'
        << "# engine_rule_source,BattleSession\n"
        << "# prng," << PRNG_VERSION << '\n'
        << "# seed," << options.seed << '\n'
        << "# mode," << modeName(options.mode) << '\n'
        << "# policy," << policyName(options.policy) << '\n'
        << "# level," << static_cast<unsigned>(options.level) << '\n'
        << "# trials_per_scenario," << options.trials << '\n'
        << "# max_turns," << options.maxTurns << '\n'
        << "# species_count," << static_cast<unsigned>(options.speciesCount) << '\n'
        << "# generator_version," << provenance.generatorVersion << '\n'
        << "# fixture_sha256,creature_data.hpp,"
        << provenance.creatureSha256 << '\n'
        << "# fixture_sha256,move_data.hpp," << provenance.moveSha256 << '\n'
        << "# fixture_sha256,battle_preset_data.hpp,"
        << provenance.presetSha256 << '\n';
}

std::string csvField(const std::string &value)
{
    if (value.find_first_of(",\"\r\n") == std::string::npos) return value;
    std::string escaped = "\"";
    for (char character : value) {
        if (character == '\"') escaped += '\"';
        escaped += character;
    }
    escaped += '\"';
    return escaped;
}

std::string teamText(const uint8_t *species, uint8_t count)
{
    std::ostringstream out;
    for (uint8_t slot = 0; slot < count; ++slot) {
        if (slot != 0) out << '|';
        out << static_cast<unsigned>(species[slot]);
    }
    return out.str();
}

std::string moveUsesText(const std::vector<MoveUse> &uses, uint8_t side)
{
    std::array<uint32_t, 256> counts{};
    for (const MoveUse &use : uses) {
        if (use.side == side) counts[use.moveId] += use.count;
    }
    std::ostringstream out;
    bool first = true;
    for (uint16_t move = 0; move < counts.size(); ++move) {
        if (counts[move] == 0) continue;
        if (!first) out << '|';
        first = false;
        out << move << ':' << counts[move];
    }
    return out.str();
}

const char *kindName(uint8_t kind)
{
    switch (static_cast<battle::ResultKind>(kind)) {
    case battle::ResultKind::None: return "none";
    case battle::ResultKind::Attack: return "attack";
    case battle::ResultKind::Switch: return "switch";
    case battle::ResultKind::Gather: return "gather";
    case battle::ResultKind::Escape: return "escape";
    case battle::ResultKind::Skip: return "skip";
    case battle::ResultKind::EndTurn: return "end-turn";
    case battle::ResultKind::UseItem: return "use-item";
    }
    return "unknown";
}

const char *outcomeName(uint8_t outcome)
{
    switch (static_cast<battle::Outcome>(outcome)) {
    case battle::Outcome::None: return "none";
    case battle::Outcome::Win: return "win";
    case battle::Outcome::Lose: return "loss";
    case battle::Outcome::Escaped: return "escaped";
    case battle::Outcome::Gathered: return "gathered";
    case battle::Outcome::Fled: return "fled";
    }
    return "unknown";
}

const char *switchReasonName(uint8_t reason)
{
    switch (reason) {
    case 1: return "improved-survival";
    case 2: return "forced-replacement";
    default: return "none";
    }
}

struct Aggregate {
    uint32_t matches = 0;
    uint32_t wins = 0;
    uint32_t losses = 0;
    uint32_t timeouts = 0;
    uint64_t turns = 0;
    uint64_t playerDamage = 0;
    uint64_t opponentDamage = 0;
    uint64_t switches[2] = {};
    std::map<std::pair<uint8_t, uint8_t>, uint64_t> moveUses;
};

using AggregateKey = std::tuple<std::string, uint8_t, std::string, std::string>;

} // namespace

bool readFixtureProvenance(const std::string &manifestPath,
                           FixtureProvenance &provenance,
                           std::string &error)
{
    error.clear();
    std::ifstream manifest(manifestPath);
    if (!manifest) {
        error = "cannot open generated-data manifest: " + manifestPath;
        return false;
    }

    FixtureProvenance next;
    std::string line;
    while (std::getline(manifest, line)) {
        if (line.find("\"name\":\"cgfx-tools\"") != std::string::npos) {
            next.generatorVersion = jsonStringField(line, "version");
        }
        const std::string path = jsonStringField(line, "path");
        if (path.empty()) continue;
        const std::string hash = jsonStringField(line, "sha256");
        if (path == "tst/fxdatatest/generated/creature_data.hpp") {
            next.creatureSha256 = hash;
        } else if (path == "tst/fxdatatest/generated/move_data.hpp") {
            next.moveSha256 = hash;
        } else if (path == "tst/fxdatatest/generated/battle_preset_data.hpp") {
            next.presetSha256 = hash;
        }
    }

    if (!manifest.eof() || next.generatorVersion.empty() ||
        next.creatureSha256.empty() || next.moveSha256.empty() ||
        next.presetSha256.empty()) {
        error = "generated-data manifest is missing simulator fixture provenance";
        return false;
    }
    provenance = std::move(next);
    return true;
}

std::string renderMatchesCsv(const Options &options,
                             const BatchSummary &summary,
                             const FixtureProvenance &provenance)
{
    std::ostringstream out;
    writeMetadata(out, options, provenance);
    out << "match,scenario,trial,policy,prng,match_seed,level,player_team,"
           "opponent_team,outcome,timeout,turns,player_damage,opponent_damage,"
           "player_move_uses,opponent_move_uses,player_hp,opponent_hp,"
           "player_switches,opponent_switches\n";
    for (const MatchRecord &record : summary.matches) {
        out << record.ordinal << ',' << csvField(record.scenario) << ','
            << record.trial << ',' << policyName(record.policy) << ','
            << PRNG_VERSION << ',' << record.seed << ','
            << static_cast<unsigned>(record.level) << ','
            << csvField(teamText(record.playerSpecies, record.playerCount)) << ','
            << csvField(teamText(record.opponentSpecies, record.opponentCount))
            << ',' << statusName(record.status) << ','
            << (record.status == MatchStatus::Timeout ? 1 : 0) << ','
            << record.turns << ',' << record.playerDamage << ','
            << record.opponentDamage << ','
            << csvField(moveUsesText(record.moveUses, 0)) << ','
            << csvField(moveUsesText(record.moveUses, 1)) << ','
            << record.playerHp << ',' << record.opponentHp << ','
            << record.switchCount[0] << ',' << record.switchCount[1] << '\n';
    }
    return out.str();
}

std::string renderSummaryCsv(const Options &options,
                             const BatchSummary &summary,
                             const FixtureProvenance &provenance)
{
    std::map<AggregateKey, Aggregate> aggregates;
    for (const MatchRecord &record : summary.matches) {
        const AggregateKey key = {
            record.scenario,
            static_cast<uint8_t>(record.policy),
            teamText(record.playerSpecies, record.playerCount),
            teamText(record.opponentSpecies, record.opponentCount),
        };
        Aggregate &aggregate = aggregates[key];
        ++aggregate.matches;
        if (record.status == MatchStatus::Win) ++aggregate.wins;
        else if (record.status == MatchStatus::Loss) ++aggregate.losses;
        else ++aggregate.timeouts;
        aggregate.turns += record.turns;
        aggregate.playerDamage += record.playerDamage;
        aggregate.opponentDamage += record.opponentDamage;
        aggregate.switches[0] += record.switchCount[0];
        aggregate.switches[1] += record.switchCount[1];
        for (const MoveUse &use : record.moveUses) {
            aggregate.moveUses[{use.side, use.moveId}] += use.count;
        }
    }

    std::ostringstream out;
    writeMetadata(out, options, provenance);
    out << "scenario,policy,player_team,opponent_team,matches,player_wins,"
           "player_losses,timeouts,turns_total,player_damage_total,"
           "opponent_damage_total,player_move_use_counts,"
           "opponent_move_use_counts,player_switches,opponent_switches\n";
    for (const auto &entry : aggregates) {
        const auto &key = entry.first;
        const Aggregate &aggregate = entry.second;
        std::array<uint64_t, 256> playerUses{};
        std::array<uint64_t, 256> opponentUses{};
        for (const auto &use : aggregate.moveUses) {
            (use.first.first == 0 ? playerUses : opponentUses)[use.first.second]
                = use.second;
        }
        auto encode = [](const std::array<uint64_t, 256> &uses) {
            std::ostringstream text;
            bool first = true;
            for (uint16_t move = 0; move < uses.size(); ++move) {
                if (uses[move] == 0) continue;
                if (!first) text << '|';
                first = false;
                text << move << ':' << uses[move];
            }
            return text.str();
        };
        out << csvField(std::get<0>(key)) << ','
            << policyName(static_cast<Policy>(std::get<1>(key))) << ','
            << csvField(std::get<2>(key)) << ','
            << csvField(std::get<3>(key)) << ',' << aggregate.matches << ','
            << aggregate.wins << ',' << aggregate.losses << ','
            << aggregate.timeouts << ',' << aggregate.turns << ','
            << aggregate.playerDamage << ',' << aggregate.opponentDamage << ','
            << csvField(encode(playerUses)) << ','
            << csvField(encode(opponentUses)) << ','
            << aggregate.switches[0] << ',' << aggregate.switches[1] << '\n';
    }
    return out.str();
}

bool writeReports(const std::string &directory, const Options &options,
                  const BatchSummary &summary,
                  const FixtureProvenance &provenance,
                  std::string &error)
{
    error.clear();
    std::error_code filesystemError;
    std::filesystem::create_directories(directory, filesystemError);
    if (filesystemError) {
        error = "cannot create report directory: " + filesystemError.message();
        return false;
    }

    const std::filesystem::path root(directory);
    const std::string matches = renderMatchesCsv(options, summary, provenance);
    const std::string totals = renderSummaryCsv(options, summary, provenance);
    std::ofstream raw(root / "matches.csv", std::ios::binary | std::ios::trunc);
    if (!raw || !(raw << matches)) {
        error = "cannot write " + (root / "matches.csv").string();
        return false;
    }
    raw.close();
    if (!raw) {
        error = "failed while closing " + (root / "matches.csv").string();
        return false;
    }
    std::ofstream aggregate(root / "summary.csv",
                            std::ios::binary | std::ios::trunc);
    if (!aggregate || !(aggregate << totals)) {
        error = "cannot write " + (root / "summary.csv").string();
        return false;
    }
    aggregate.close();
    if (!aggregate) {
        error = "failed while closing " + (root / "summary.csv").string();
        return false;
    }
    return true;
}

void writeTrace(std::ostream &out, const ReplayResult &result)
{
    out << "# scenario," << result.match.scenario << '\n'
        << "# policy," << policyName(result.match.policy) << '\n'
        << "# match_seed," << result.match.seed << '\n'
        << "# level," << static_cast<unsigned>(result.match.level) << '\n'
        << "# player_team,"
        << teamText(result.match.playerSpecies, result.match.playerCount) << '\n'
        << "# opponent_team,"
        << teamText(result.match.opponentSpecies, result.match.opponentCount)
        << '\n'
        << "# result," << statusName(result.match.status) << '\n'
        << "# turns," << result.match.turns << '\n'
        << "# player_damage," << result.match.playerDamage << '\n'
        << "# opponent_damage," << result.match.opponentDamage << '\n'
        << "# player_switches," << result.match.switchCount[0] << '\n'
        << "# opponent_switches," << result.match.switchCount[1] << '\n'
        << "step,kind,actor,index,flags,switch_reason,outcome,player_species,"
           "opponent_species,player_hp_before,player_hp_after,"
           "opponent_hp_before,opponent_hp_after\n";
    for (const TraceEvent &event : result.trace) {
        out << event.step << ',' << kindName(event.kind) << ','
            << (event.actor == 0 ? "player" : "opponent") << ','
            << static_cast<unsigned>(event.index) << ','
            << static_cast<unsigned>(event.flags) << ','
            << switchReasonName(event.switchReason) << ','
            << outcomeName(event.outcome) << ','
            << static_cast<unsigned>(event.speciesBefore[0]) << ','
            << static_cast<unsigned>(event.speciesBefore[1]) << ','
            << static_cast<unsigned>(event.hpBefore[0]) << ','
            << static_cast<unsigned>(event.hpAfter[0]) << ','
            << static_cast<unsigned>(event.hpBefore[1]) << ','
            << static_cast<unsigned>(event.hpAfter[1]) << '\n';
    }
}

} // namespace battle_sim

#endif
