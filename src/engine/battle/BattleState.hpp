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

// Active combatant. The legacy representation caches the packed move, its
// stats/status/stage state and the semantic move IDs. The duel has none of
// those mechanics, so the flag build keeps only the fields it actually reads:
// one authored attack type per slot (folded at the view/resolver boundary) plus
// the authored move ID that selects custom/saved kits. 255 marks an empty slot.
struct Combatant {
    uint8_t id;
    DualType types;
    uint8_t level, hp, maxHp;
#ifdef CGFX_DUEL_BATTLE
    uint8_t moveTypes[4];
#else
    stats_t stats;
    Move moves[4];
#endif
    uint8_t moveIds[4];
#ifndef CGFX_DUEL_BATTLE
    StatusEffect status;
    StatModifer statMods;
    uint8_t effectTurns = 0; // Two bits per status slot; zero is untimed.
#endif
};

// Cached bench member. The duel reads only identity/level/HP (SWAP availability,
// pips, roster); the defensive profile is a legacy switch-bench cache.
struct BenchSlot {
    uint8_t id, level, hp;
#ifndef CGFX_DUEL_BATTLE
    DualType types;
    uint8_t defense, specialDefense;
#endif
};

struct GatherState {
    uint8_t progress, need, fleeTurns, tierRate;
};

struct BattleState {
    Combatant active[2];
    BenchSlot bench[2][PARTY_SIZE - 1];
    uint8_t partyCount[2], activeSlot[2];
    // Two bits per move slot, indexed by original party slot (not bench order).
    uint8_t moveUsesSpent[2][PARTY_SIZE] = {};
#ifndef CGFX_DUEL_BATTLE
    // Stat-stage modifiers are a legacy-mechanics field. The duel has no
    // stages, so the flag build reclaims these 12 bytes; moveUsesSpent is kept
    // because the duel aliases it for per-member attack-type masks.
    uint16_t partyModifiers[2][PARTY_SIZE] = {};
#endif
    GatherState gather;
    uint8_t gatherable : 1;
    uint8_t trainer : 1;
    uint8_t over : 1;
    // Opponent-only guard: set when the AI switches, cleared when it acts.
    uint8_t switchLockMask : 2;
    uint8_t trainerId;
#ifdef CGFX_DUEL_BATTLE
    // Duel compact-UI facts (CreatureGathererFX-d73i.2): the 1-based round, the
    // result caption kind (duel::Feedback), and the last round's two damage
    // amounts and two revealed move types (folded; 255 = duel::NO_TYPE).
    uint8_t round;
    uint8_t feedback;
    uint8_t damage[2];
    uint8_t reveal[2];
#endif
};

#ifdef __AVR__
#ifdef CGFX_DUEL_BATTLE
static_assert(sizeof(Combatant) == 13, "duel active combatant AVR contract");
static_assert(sizeof(BenchSlot) == 3, "duel bench slot AVR contract");
static_assert(sizeof(BattleState) == 60, "duel battle state AVR contract");
#else
static_assert(sizeof(Combatant) == 36, "active combatant AVR contract");
static_assert(sizeof(BattleState) == 124, "battle state AVR contract");
#endif
#endif

} // namespace battle
