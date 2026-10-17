#pragma once

#include "BattleTypes.hpp"
#include "../../lib/Type.hpp"
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

// Active combatant. The duel keeps only the fields it reads: the creature's
// folded 5-type attack mask (bit i = duel type i; 0 = no usable attack), built
// from the authored kit at the setup transition.
struct Combatant {
    uint8_t id;
    DualType types;
    uint8_t level, hp, maxHp;
    uint8_t attackMask;
};

// Cached bench member. The duel reads only identity/level/HP (SWAP availability,
// pips, roster).
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
    // Two bits per move slot, indexed by original party slot (not bench order).
    // The duel aliases it for per-member attack-type masks.
    uint8_t moveUsesSpent[2][PARTY_SIZE] = {};
    GatherState gather;
    uint8_t gatherable : 1;
    uint8_t trainer : 1;
    uint8_t over : 1;
    // Opponent-only guard: set when the AI switches, cleared when it acts.
    uint8_t switchLockMask : 2;
    uint8_t trainerId;
    // Duel compact-UI facts (CreatureGathererFX-d73i.2): the 1-based round, the
    // result caption kind (duel::Feedback), and the last round's two damage
    // amounts and two revealed move types (folded; 255 = duel::NO_TYPE).
    uint8_t round;
    uint8_t feedback;
    uint8_t damage[2];
    uint8_t reveal[2];
};

#ifdef __AVR__
static_assert(sizeof(Combatant) == 6, "duel active combatant AVR contract");
static_assert(sizeof(BenchSlot) == 3, "duel bench slot AVR contract");
static_assert(sizeof(BattleState) == 46, "duel battle state AVR contract");
#endif

} // namespace battle
