#pragma once

#include "BattleTypes.hpp"
#include "../../lib/Effect.hpp"
#include "../../lib/Type.hpp"

#if defined(CGFX_BATTLE_COMPACT_RESULT) && !defined(CGFX_BATTLE_NO_FACTS)
#error "CGFX_BATTLE_COMPACT_RESULT requires CGFX_BATTLE_NO_FACTS"
#endif

namespace battle {

enum class ResultKind : uint8_t {
    None, Attack, Switch, Gather, Escape, Skip, EndTurn, UseItem
};
// Draw is appended (not inserted) so existing serialized outcome values keep
// their numeric identity. It is the duel's simultaneous-KO result; it grants
// no victory and uses defeat recovery like a loss.
enum class Outcome : uint8_t { None, Win, Lose, Escaped, Gathered, Fled, Draw };

enum : uint8_t {
    SELF_HIT = 1u << 0, REFUSED = 1u << 1, STATUS_SKIPPED = 1u << 2,
    PLAYER_FAINTED = 1u << 3, OPPONENT_FAINTED = 1u << 4, FORCED_SWITCH = 1u << 5,
    PHYSICAL_MOVE = 1u << 6
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
#ifdef CGFX_BATTLE_COMPACT_RESULT
    uint8_t hpBefore[2];
#else
    Modifier effectiveness;
    uint8_t speciesBefore[2], maxHpBefore[2];
    uint8_t hpBefore[2], hpAfter[2];
    uint8_t progressBefore, progressAfter;
    Consequence consequences[4];
#endif
};

// Reset the caller-owned POD explicitly. An aggregate with nonzero default
// fact sentinels otherwise gives AVR a twelve-byte initializer in SRAM.
inline void resetActionResult(ActionResult &result) {
    result.kind = ResultKind::None;
    result.actor = Side::Player;
    result.index = 255;
    result.flags = 0;
    result.outcome = Outcome::None;
#ifdef CGFX_BATTLE_COMPACT_RESULT
    result.hpBefore[0] = result.hpBefore[1] = 0;
#else
    result.effectiveness = Modifier::Same;
    for (uint8_t side = 0; side < 2; ++side) {
        result.speciesBefore[side] = result.maxHpBefore[side] = 0;
        result.hpBefore[side] = result.hpAfter[side] = 0;
    }
    result.progressBefore = result.progressAfter = 0;
#ifndef CGFX_BATTLE_NO_FACTS
    for (uint8_t i = 0; i < 4; ++i) {
        result.consequences[i].effect = Effect::NONE;
        result.consequences[i].side = 255;
        result.consequences[i].value = 0;
    }
#endif
#endif
}

static_assert(sizeof(Consequence) == 3, "semantic consequence contract");
#ifdef CGFX_BATTLE_COMPACT_RESULT
static_assert(sizeof(ActionResult) == 7, "compact shipping result contract");
#else
static_assert(sizeof(ActionResult) == 28, "one resident action result contract");
#endif

} // namespace battle
