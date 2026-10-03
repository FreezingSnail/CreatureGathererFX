#pragma once

#include "BattleTypes.hpp"
#include "../../lib/Effect.hpp"
#include "../../lib/Type.hpp"

namespace battle {

enum class ResultKind : uint8_t { None, Attack, Switch, Gather, Escape, Skip, EndTurn };
enum class Outcome : uint8_t { None, Win, Lose, Escaped, Gathered, Fled };

enum : uint8_t {
    SELF_HIT = 1u << 0, REFUSED = 1u << 1, STATUS_SKIPPED = 1u << 2,
    PLAYER_FAINTED = 1u << 3, OPPONENT_FAINTED = 1u << 4, FORCED_SWITCH = 1u << 5
};

// Successful effect facts, or sequential post-tick HP facts. Unused facts
// never name a side; this is the fixed capacity of current two-slot mechanics.
struct Consequence {
    Effect effect;
    uint8_t side;
    uint8_t value;
};

struct ActionResult {
    ResultKind kind;
    Side actor;
    uint8_t index;
    uint8_t flags;
    Outcome outcome;
    Modifier effectiveness;
    uint8_t speciesBefore[2], maxHpBefore[2];
    uint8_t hpBefore[2], hpAfter[2];
    uint8_t progressBefore, progressAfter;
    Consequence consequences[4];
};

// Reset the caller-owned POD explicitly. An aggregate with nonzero default
// fact sentinels otherwise gives AVR a twelve-byte initializer in SRAM.
inline void resetActionResult(ActionResult &result) {
    result.kind = ResultKind::None;
    result.actor = Side::Player;
    result.index = 255;
    result.flags = 0;
    result.outcome = Outcome::None;
    result.effectiveness = Modifier::Same;
    for (uint8_t side = 0; side < 2; ++side) {
        result.speciesBefore[side] = result.maxHpBefore[side] = 0;
        result.hpBefore[side] = result.hpAfter[side] = 0;
    }
    result.progressBefore = result.progressAfter = 0;
    for (uint8_t i = 0; i < 4; ++i) {
        result.consequences[i].effect = Effect::NONE;
        result.consequences[i].side = 255;
        result.consequences[i].value = 0;
    }
}

static_assert(sizeof(Consequence) == 3, "semantic consequence contract");
static_assert(sizeof(ActionResult) == 28, "one resident action result contract");

} // namespace battle
