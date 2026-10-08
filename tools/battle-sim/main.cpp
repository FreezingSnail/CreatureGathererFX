#include <charconv>
#include <stdint.h>

#include <iostream>
#include <string>
#include <system_error>
#include <vector>

#include "BatchRunner.hpp"
#include "SimulatorReports.hpp"
#include "../../src/player/Player.hpp"

Player player;

namespace {

void usage(std::ostream &out)
{
    out << "Usage: battle-sim [options]\n"
        << "  --mode pairwise|random-3v3|anchors\n"
        << "  --seed N                 base seed (default 1)\n"
        << "  --level N                common species level 1..31 (default 10)\n"
        << "  --trials N               trials per matchup/scenario (default 1)\n"
        << "  --policy greedy|random-valid-move|tactical|switch-tactical|both (default both)\n"
        << "  --max-turns N            timeout bound (default 100)\n"
        << "  --species-count N        use fixture IDs 0..N-1 (default 32, max 64)\n"
        << "  --output-dir PATH        report directory (default build/balance)\n"
        << "  --replay-scenario NAME   pair_NN_NN, random_3v3, opening, switch_drill, or utility\n"
        << "  --replay-seed N          recorded per-match seed\n"
        << "  --replay-player-team IDS species IDs separated by | for random_3v3\n"
        << "  --replay-opponent-team IDS species IDs separated by | for random_3v3\n"
        << "  --help                    print this help\n";
}

struct CommandOptions {
    battle_sim::Options batch;
    std::string outputDirectory = "build/balance";
    std::string replayScenario;
    uint64_t replaySeed = 0;
    bool hasReplaySeed = false;
    std::vector<uint8_t> replayPlayerTeam;
    std::vector<uint8_t> replayOpponentTeam;
};

bool parseUnsigned(const char *text, uint64_t maximum, uint64_t &value)
{
    if (text == nullptr || *text == '\0' || *text == '-') return false;
    const char *end = text;
    while (*end != '\0') ++end;
    const auto parsed = std::from_chars(text, end, value);
    return parsed.ec == std::errc{} && parsed.ptr == end && value <= maximum;
}

bool parseTeam(const char *text, std::vector<uint8_t> &team)
{
    const std::string value(text == nullptr ? "" : text);
    if (value.empty()) return false;
    size_t first = 0;
    while (first < value.size()) {
        const size_t separator = value.find('|', first);
        const size_t last = separator == std::string::npos
            ? value.size() : separator;
        uint64_t species = 0;
        if (!parseUnsigned(value.substr(first, last - first).c_str(), 63,
                           species) || team.size() >= 3) {
            return false;
        }
        team.push_back(static_cast<uint8_t>(species));
        if (separator == std::string::npos) return true;
        first = separator + 1;
    }
    return false;
}

bool parseOptions(int argc, char **argv, CommandOptions &command,
                  bool &showHelp, std::string &error)
{
    battle_sim::Options &options = command.batch;
    using battle_sim::Mode;
    using battle_sim::Policy;
    showHelp = false;
    for (int index = 1; index < argc; ++index) {
        const std::string option = argv[index];
        if (option == "--help") {
            showHelp = true;
            return true;
        }
        if (index + 1 >= argc) {
            error = "missing value for " + option;
            return false;
        }
        const char *value = argv[++index];
        if (option == "--output-dir") {
            command.outputDirectory = value;
            if (command.outputDirectory.empty()) {
                error = "output directory cannot be empty";
                return false;
            }
        } else if (option == "--replay-scenario") {
            command.replayScenario = value;
            if (command.replayScenario.empty()) {
                error = "replay scenario cannot be empty";
                return false;
            }
        } else if (option == "--replay-player-team") {
            if (!parseTeam(value, command.replayPlayerTeam)) {
                error = "invalid replay player team; use up to three IDs separated by |";
                return false;
            }
        } else if (option == "--replay-opponent-team") {
            if (!parseTeam(value, command.replayOpponentTeam)) {
                error = "invalid replay opponent team; use up to three IDs separated by |";
                return false;
            }
        } else if (option == "--mode") {
            if (std::string(value) == "pairwise") options.mode = Mode::Pairwise;
            else if (std::string(value) == "random-3v3") options.mode = Mode::Random3v3;
            else if (std::string(value) == "anchors") options.mode = Mode::Anchors;
            else {
                error = "mode must be pairwise, random-3v3, or anchors";
                return false;
            }
        } else if (option == "--policy") {
            if (std::string(value) == "greedy") options.policy = Policy::Greedy;
            else if (std::string(value) == "random-valid-move")
                options.policy = Policy::RandomValidMove;
            else if (std::string(value) == "tactical") options.policy = Policy::Tactical;
            else if (std::string(value) == "switch-tactical") options.policy = Policy::SwitchTactical;
            else if (std::string(value) == "both") options.policy = Policy::Both;
            else {
                error = "policy must be greedy, random-valid-move, tactical, switch-tactical, or both";
                return false;
            }
        } else {
            uint64_t parsed = 0;
            uint64_t maximum = UINT64_MAX;
            if (option == "--level") maximum = 31;
            else if (option == "--trials" || option == "--max-turns") maximum = 65535;
            else if (option == "--species-count") maximum = 64;
            else if (option == "--replay-seed") maximum = UINT64_MAX;
            else if (option != "--seed") {
                error = "unknown option: " + option;
                return false;
            }
            if (!parseUnsigned(value, maximum, parsed)) {
                error = "invalid numeric value for " + option;
                return false;
            }
            if (option == "--seed") options.seed = parsed;
            else if (option == "--level") options.level = static_cast<uint8_t>(parsed);
            else if (option == "--trials") options.trials = static_cast<uint16_t>(parsed);
            else if (option == "--max-turns") options.maxTurns = static_cast<uint16_t>(parsed);
            else if (option == "--species-count") options.speciesCount = static_cast<uint8_t>(parsed);
            else {
                command.replaySeed = parsed;
                command.hasReplaySeed = true;
            }
        }
    }
    const bool hasPlayerTeam = !command.replayPlayerTeam.empty();
    const bool hasOpponentTeam = !command.replayOpponentTeam.empty();
    if (hasPlayerTeam != hasOpponentTeam ||
        ((hasPlayerTeam || hasOpponentTeam) &&
         command.replayScenario != "random_3v3")) {
        error = "replay teams must be supplied together for random_3v3";
        return false;
    }
    return true;
}

} // namespace

int main(int argc, char **argv)
{
    CommandOptions command;
    bool showHelp = false;
    std::string error;
    if (!parseOptions(argc, argv, command, showHelp, error)) {
        std::cerr << "battle-sim: " << error << '\n';
        usage(std::cerr);
        return 2;
    }
    if (showHelp) {
        usage(std::cout);
        return 0;
    }

    if (!command.replayScenario.empty() || command.hasReplaySeed) {
        if (command.replayScenario.empty() || !command.hasReplaySeed) {
            std::cerr << "battle-sim: replay needs both --replay-scenario and --replay-seed\n";
            return 2;
        }
        battle_sim::ReplayRequest request;
        request.scenario = command.replayScenario;
        request.seed = command.replaySeed;
        request.level = command.batch.level;
        request.policy = command.batch.policy;
        request.maxTurns = command.batch.maxTurns;
        request.playerTeam = command.replayPlayerTeam;
        request.opponentTeam = command.replayOpponentTeam;
        battle_sim::ReplayResult replay;
        if (!battle_sim::runReplay(request, replay, error)) {
            std::cerr << "battle-sim: " << error << '\n';
            return 2;
        }
        battle_sim::writeTrace(std::cout, replay);
        return 0;
    }

    battle_sim::BatchSummary summary;
    if (!battle_sim::runBatch(command.batch, summary, error)) {
        std::cerr << "battle-sim: " << error << '\n';
        return 2;
    }

    battle_sim::FixtureProvenance provenance;
    if (!battle_sim::readFixtureProvenance("fxdata/generated/manifest.json",
                                          provenance, error)) {
        std::cerr << "battle-sim: " << error << '\n';
        return 2;
    }
    if (!battle_sim::writeReports(command.outputDirectory, command.batch,
                                  summary, provenance, error)) {
        std::cerr << "battle-sim: " << error << '\n';
        return 2;
    }
    std::cout << "matches=" << summary.matches.size() << '\n'
              << "raw_report=" << command.outputDirectory << "/matches.csv\n"
              << "summary_report=" << command.outputDirectory << "/summary.csv\n";
    return 0;
}
