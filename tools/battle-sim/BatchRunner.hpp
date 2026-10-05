#pragma once

#if defined(BATTLE_SIMULATOR) && !defined(__AVR__)

#include <stdint.h>

#include <string>
#include <vector>

#include "../../src/engine/battle/ActionResult.hpp"

namespace battle_sim {

enum class Mode : uint8_t { Pairwise, Random3v3, Anchors };
enum class Policy : uint8_t {
    Greedy, RandomValidMove, Both, Tactical, SwitchTactical
};
enum class MatchStatus : uint8_t { Win, Loss, Timeout };

struct Options {
    Mode mode = Mode::Anchors;
    uint64_t seed = 1;
    uint8_t level = 10;
    uint16_t trials = 1;
    Policy policy = Policy::Both;
    uint16_t maxTurns = 100;
    uint8_t speciesCount = 32;
};

struct MoveUse {
    uint8_t side = 0;
    uint8_t moveId = 255;
    uint32_t count = 0;
};

struct MatchRecord {
    uint32_t ordinal = 0;
    uint16_t trial = 0;
    uint64_t seed = 0;
    uint8_t level = 0;
    std::string scenario;
    Policy policy = Policy::Greedy;
    uint8_t playerCount = 0;
    uint8_t opponentCount = 0;
    uint8_t playerSpecies[3] = {};
    uint8_t opponentSpecies[3] = {};
    MatchStatus status = MatchStatus::Timeout;
    uint16_t turns = 0;
    uint16_t playerHp = 0;
    uint16_t opponentHp = 0;
    uint16_t switchCount[2] = {};
    uint32_t playerDamage = 0;
    uint32_t opponentDamage = 0;
    std::vector<MoveUse> moveUses;
};

struct TraceEvent {
    uint32_t step = 0;
    uint8_t kind = 0;
    uint8_t actor = 0;
    uint8_t index = 255;
    uint8_t flags = 0;
    uint8_t switchReason = 0;
    uint8_t outcome = 0;
    uint8_t speciesBefore[2] = {};
    uint8_t hpBefore[2] = {};
    uint8_t hpAfter[2] = {};
};

struct ReplayRequest {
    std::string scenario;
    uint64_t seed = 0;
    uint8_t level = 10;
    Policy policy = Policy::Greedy;
    uint16_t maxTurns = 100;
    std::vector<uint8_t> playerTeam;
    std::vector<uint8_t> opponentTeam;
};

struct ReplayResult {
    MatchRecord match;
    std::vector<TraceEvent> trace;
};

struct BatchSummary {
    std::vector<MatchRecord> matches;
    std::vector<uint32_t> speciesAppearances;
};

constexpr const char *PRNG_VERSION = "xorshift32-v1";
constexpr const char *SIMULATOR_VERSION = "3";
constexpr const char *REPORT_SCHEMA_VERSION = "2";

const char *modeName(Mode mode);
const char *policyName(Policy policy);
const char *statusName(MatchStatus status);
bool runBatch(const Options &options, BatchSummary &summary,
              std::string &error);
bool runReplay(const ReplayRequest &request, ReplayResult &result,
               std::string &error);

} // namespace battle_sim

#endif
