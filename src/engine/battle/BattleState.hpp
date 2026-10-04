#pragma once

#include "BattleTypes.hpp"
#include "../../lib/Move.hpp"
#include "../../lib/Type.hpp"
#include "../../lib/Stats.hpp"
#include "../../lib/StatusEffect.hpp"
#include "../../lib/StatModifier.hpp"
#include "../../values.hpp"

namespace battle {

// UseItem carries a consumable id in index. Append only: serialized action
// values Attack through Skip remain stable; item kind is implicit.
enum class ActionKind : uint8_t {
    Attack, Switch, Gather, Escape, Skip, UseItem
};

struct BattleAction {
    ActionKind kind;
    uint8_t index;
};

struct TurnPlan {
    BattleAction action[2];
};

// Packed moves and their semantic IDs are both loaded at battle transitions.
struct Combatant {
    uint8_t id;
    DualType types;
    uint8_t level, hp, maxHp;
    stats_t stats;
    Move moves[4];
    uint8_t moveIds[4];
    StatusEffect status;
    StatModifer statMods;
};

struct BenchSlot {
    uint8_t id, level, hp;
};

struct GatherState {
    uint8_t progress, need, fleeTurns, tierRate;
};

struct BattleState {
    Combatant active[2];
    BenchSlot bench[2][PARTY_SIZE - 1];
    uint8_t partyCount[2], activeSlot[2];
    GatherState gather;
    bool gatherable, trainer, over;
    uint8_t trainerId;
};

#ifdef __AVR__
static_assert(sizeof(Combatant) == 35, "active combatant AVR contract");
static_assert(sizeof(BattleState) == 94, "battle state AVR contract");
#endif

} // namespace battle
