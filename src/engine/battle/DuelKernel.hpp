#pragma once
// RPS duel battle kernel (CreatureGathererFX-d73i.1, spike step 1).
//
// Hidden-simultaneous-pick, 5-type full-tournament duel over teams of three.
// Pure fixed-integer logic: no FX reads, no globals, no float. Includes the
// kernel header directly under CGFX_DUEL_BATTLE only, so legacy builds are
// byte-identical. Rules are pinned by docs/duel-system-spike-plan.md and the
// mock reference (mock/battle-mock.js, mock/matrices.js id "5").

#include <stdint.h>
#include "BattleRng.hpp"

namespace duel {

constexpr uint8_t TYPE_COUNT = 5;
constexpr uint8_t PARTY_MAX = 3;
constexpr uint8_t HISTORY_CAP = 8;
constexpr uint8_t OPTION_CAP = 4;

// Duel types in mock/matrices.js id "5" order.
enum class DType : uint8_t {
    Spirit = 0,
    Water = 1,
    Earth = 2,
    Fire = 3,
    Plant = 4,
    None = 255,
};

// Fold an authored 8-type id (src/lib/Type.hpp order: SPIRIT0 WATER1 WIND2
// EARTH3 FIRE4 LIGHTNING5 PLANT6 ELDER7) to the 5 duel types.
// lightning->fire, wind->water, elder->spirit; the rest stay.
inline uint8_t foldType(uint8_t authoredType)
{
    static const uint8_t fold[8] = {0, 1, 1, 2, 3, 3, 4, 0};
    return authoredType < 8 ? fold[authoredType] : static_cast<uint8_t>(DType::None);
}

enum : uint8_t { REL_LOSE = 0, REL_TRADE = 1, REL_WIN = 2 };

#if defined(CGFX_DUEL_BATTLE) && defined(__AVR__)
// Canonical AVR duel relation source (selected by the step-1 whole-image A/B):
// the 25-cell chart is loaded once at encounter entry into a resident buffer and
// every lookup reads RAM. Semantics are identical to the PROGMEM win masks,
// which remain for host/native kernel tests. Evidence in build/reports/d73i/.
extern uint8_t duelChart[TYPE_COUNT * TYPE_COUNT];
inline uint8_t relation(uint8_t moveType, uint8_t defendType)
{
    if (moveType >= TYPE_COUNT || defendType >= TYPE_COUNT) return REL_TRADE;
    return duelChart[static_cast<uint8_t>(moveType * TYPE_COUNT + defendType)];
}
#else
// Five PROGMEM win masks: bit b of entry t is set when type t beats type b.
// Relation is the full tournament's 10 edges; the lose relation is the mirror.
#ifdef __AVR__
#include <avr/pgmspace.h>
constexpr uint8_t WIN_MASKS[TYPE_COUNT] PROGMEM = {0x08, 0x0D, 0x19, 0x10, 0x03};
inline uint8_t winMask(uint8_t type)
{
    return type < TYPE_COUNT ? pgm_read_byte(&WIN_MASKS[type]) : 0;
}
#else
constexpr uint8_t WIN_MASKS[TYPE_COUNT] = {0x08, 0x0D, 0x19, 0x10, 0x03};
inline uint8_t winMask(uint8_t type)
{
    return type < TYPE_COUNT ? WIN_MASKS[type] : 0;
}
#endif

// Relation of a move type against a defending creature type (0/1/2).
inline uint8_t relation(uint8_t moveType, uint8_t defendType)
{
    if (moveType >= TYPE_COUNT || defendType >= TYPE_COUNT) return REL_TRADE;
    if ((winMask(moveType) >> defendType) & 1u) return REL_WIN;
    if ((winMask(defendType) >> moveType) & 1u) return REL_LOSE;
    return REL_TRADE;
}
#endif

// Damage bases and x1.5/x0.5/x1 as tenths (fixed point, no float).
enum : uint8_t { DMG_WIN = 14, DMG_CHIP = 4, DMG_TRADE = 6 };
enum : uint8_t { MOD_HALF = 5, MOD_SAME = 10, MOD_BONUS = 15 };

inline uint8_t scaleDamage(uint8_t base, uint8_t modifier10)
{
    const uint16_t scaled =
        (static_cast<uint16_t>(base) * modifier10 + 5u) / 10u;
    return scaled == 0 ? 1 : static_cast<uint8_t>(scaled);
}

// Advantage-first creature-type matchup: any beaten defender type gives x1.5,
// otherwise any losing type gives x0.5, otherwise x1. No multiplied dual
// weakness, matching the mock.
inline uint8_t creatureModifier(uint8_t moveType, const uint8_t *types,
                                uint8_t typeCount)
{
    for (uint8_t i = 0; i < typeCount; ++i)
        if (relation(moveType, types[i]) == REL_WIN) return MOD_BONUS;
    for (uint8_t i = 0; i < typeCount; ++i)
        if (relation(moveType, types[i]) == REL_LOSE) return MOD_HALF;
    return MOD_SAME;
}

struct Member {
    uint8_t id;
    uint8_t typeCount;      // 1 or 2 distinct folded types
    uint8_t types[2];
    uint8_t hp;
    uint8_t maxHp;
    uint8_t optionCount;    // one entry per usable authored attack slot
    uint8_t options[OPTION_CAP]; // folded move types, slot order, may duplicate
};

struct Side {
    Member members[PARTY_MAX];
    uint8_t count;
    uint8_t active;         // original slot index of the active member
};

enum class ActionKind : uint8_t { Attack = 0, Switch = 1, Wait = 2 };

struct Action {
    ActionKind kind;
    uint8_t index; // attack: option index; switch: target original slot
};

enum class Persona : uint8_t { Random = 0, Reader = 1, Frequency = 2, AntiRepeat = 3 };

enum class Outcome : uint8_t {
    None = 0, Win = 1, Lose = 2, Draw = 3, Gathered = 4, Escaped = 5, Fled = 6
};

struct State {
    Side sides[2];
    uint8_t history[HISTORY_CAP]; // last revealed player attack types
    uint8_t historyCount;
    Outcome outcome;
};

enum : uint8_t { NO_TYPE = 255, NO_SLOT = 255 };

struct RoundResult {
    uint8_t kind[2];            // ActionKind value per side
    uint8_t damage[2];          // pre-hit HP removed from the resolving member
    uint8_t hpBefore[2];        // resolving (post-switch) member HP before the hit
    uint8_t activeId[2];        // resolving (post-switch) member species/preset id
    uint8_t previousActive[2];  // outgoing original slot when the side switched
    uint8_t relation;           // REL_* when both attacked, else NO_TYPE
};

inline bool sideAlive(const State &state, uint8_t side)
{
    for (uint8_t i = 0; i < state.sides[side].count; ++i)
        if (state.sides[side].members[i].hp != 0) return true;
    return false;
}

inline uint8_t firstLivingSlot(const State &state, uint8_t side)
{
    for (uint8_t i = 0; i < state.sides[side].count; ++i)
        if (state.sides[side].members[i].hp != 0) return i;
    return NO_SLOT;
}

// Promote after a faint. `preferred` restores a KO'd incoming swap when it is
// still living; otherwise the first living original slot is promoted.
inline void syncActive(State &state, uint8_t side, uint8_t preferred)
{
    Side &s = state.sides[side];
    if (s.members[s.active].hp != 0) return;
    if (preferred != NO_SLOT && preferred < s.count &&
        s.members[preferred].hp != 0) {
        s.active = preferred;
        return;
    }
    const uint8_t slot = firstLivingSlot(state, side);
    if (slot != NO_SLOT) s.active = slot;
}

inline void recordPlayerHistory(State &state, uint8_t type)
{
    if (state.historyCount < HISTORY_CAP) {
        state.history[state.historyCount] = type;
        ++state.historyCount;
        return;
    }
    for (uint8_t i = 1; i < HISTORY_CAP; ++i)
        state.history[i - 1] = state.history[i];
    state.history[HISTORY_CAP - 1] = type;
}

// Resolve one atomic round. Both sides' damages are computed against the same
// pre-hit state and then applied together, so simultaneous KO/draw is exact.
inline Outcome resolveRound(State &state, const Action actions[2],
                            battle::Rng &rng, RoundResult &out)
{
    (void)rng;
    out.kind[0] = static_cast<uint8_t>(actions[0].kind);
    out.kind[1] = static_cast<uint8_t>(actions[1].kind);
    out.damage[0] = out.damage[1] = 0;
    out.relation = NO_TYPE;

    const bool switchPick[2] = {actions[0].kind == ActionKind::Switch,
                                actions[1].kind == ActionKind::Switch};
    const bool waitPick[2] = {actions[0].kind == ActionKind::Wait,
                              actions[1].kind == ActionKind::Wait};

    uint8_t previous[2] = {NO_SLOT, NO_SLOT};

    // Apply switches first so default wins read the incoming creature's types
    // and the result tracks the incoming member's pre-hit HP, not the outgoing.
    for (uint8_t side = 0; side < 2; ++side) {
        if (!switchPick[side]) continue;
        Side &s = state.sides[side];
        const uint8_t target = actions[side].index;
        if (target >= s.count || target == s.active || s.members[target].hp == 0)
            continue;
        previous[side] = s.active;
        s.active = target;
    }
    for (uint8_t side = 0; side < 2; ++side) {
        const Member &resolving = state.sides[side].members[state.sides[side].active];
        out.hpBefore[side] = resolving.hp;
        out.activeId[side] = resolving.id;
        out.previousActive[side] = previous[side];
    }

    uint8_t moveType[2] = {NO_TYPE, NO_TYPE};
    for (uint8_t side = 0; side < 2; ++side) {
        const Member &active = state.sides[side].members[state.sides[side].active];
        if (actions[side].kind == ActionKind::Attack &&
            actions[side].index < active.optionCount) {
            moveType[side] = active.options[actions[side].index];
        }
    }

    uint8_t damage[2] = {0, 0};
    const bool bothSwitch = switchPick[0] && switchPick[1];

    if (bothSwitch) {
        // Both swaps complete and deal zero damage.
        out.relation = NO_TYPE;
    } else if (switchPick[0] || switchPick[1]) {
        // The side that did not switch lands a default win against the
        // incoming creature's type, scaled by the attacker's move matchup.
        const uint8_t attacker = switchPick[0] ? 1 : 0;
        const uint8_t defender = 1 - attacker;
        const Member &att = state.sides[attacker].members[state.sides[attacker].active];
        const Member &incoming = state.sides[defender].members[state.sides[defender].active];
        if (moveType[attacker] != NO_TYPE) {
            damage[defender] = scaleDamage(
                DMG_WIN,
                creatureModifier(moveType[attacker], incoming.types, incoming.typeCount));
        }
        out.relation = NO_TYPE;
    } else if (waitPick[0] && waitPick[1]) {
        // Both waiting: no attack, no damage.
        out.relation = NO_TYPE;
    } else if (waitPick[0] || waitPick[1]) {
        // The waiting side commits no attack and receives a default win.
        const uint8_t attacker = waitPick[0] ? 1 : 0;
        const uint8_t defender = 1 - attacker;
        const Member &victim = state.sides[defender].members[state.sides[defender].active];
        if (moveType[attacker] != NO_TYPE) {
            damage[defender] = scaleDamage(
                DMG_WIN,
                creatureModifier(moveType[attacker], victim.types, victim.typeCount));
        }
        out.relation = NO_TYPE;
    } else if (moveType[0] != NO_TYPE && moveType[1] != NO_TYPE) {
        const Member &m0 = state.sides[0].members[state.sides[0].active];
        const Member &m1 = state.sides[1].members[state.sides[1].active];
        const uint8_t rel = relation(moveType[0], moveType[1]);
        out.relation = rel;
        const uint8_t mod0 = creatureModifier(moveType[0], m1.types, m1.typeCount);
        const uint8_t mod1 = creatureModifier(moveType[1], m0.types, m0.typeCount);
        if (rel == REL_WIN) {
            damage[1] = scaleDamage(DMG_WIN, mod0);
            damage[0] = scaleDamage(DMG_CHIP, mod1);
        } else if (rel == REL_LOSE) {
            damage[0] = scaleDamage(DMG_WIN, mod1);
            damage[1] = scaleDamage(DMG_CHIP, mod0);
        } else {
            damage[0] = scaleDamage(DMG_TRADE, mod0);
            damage[1] = scaleDamage(DMG_TRADE, mod1);
        }
    } else {
        // One side has an unusable kit and cannot supply an attack. Treat it
        // like a wait so the round still terminates with a default win.
        const uint8_t attacker = moveType[0] == NO_TYPE ? 1 : 0;
        const uint8_t defender = 1 - attacker;
        if (moveType[attacker] != NO_TYPE) {
            const Member &victim = state.sides[defender].members[state.sides[defender].active];
            damage[defender] = scaleDamage(
                DMG_WIN,
                creatureModifier(moveType[attacker], victim.types, victim.typeCount));
        }
        out.relation = NO_TYPE;
    }

    // Apply both hits against the same pre-hit state, then clamp at zero.
    for (uint8_t side = 0; side < 2; ++side) {
        Member &m = state.sides[side].members[state.sides[side].active];
        m.hp = damage[side] >= m.hp ? 0 : static_cast<uint8_t>(m.hp - damage[side]);
        out.damage[side] = damage[side];
    }

    // Swap-KO reverts to the outgoing member when living; ordinary KO promotes
    // the first living original slot.
    for (uint8_t side = 0; side < 2; ++side) {
        syncActive(state, side, previous[side]);
    }

    const bool alive0 = sideAlive(state, 0);
    const bool alive1 = sideAlive(state, 1);
    if (!alive0 && !alive1) {
        state.outcome = Outcome::Draw;
    } else if (!alive1) {
        state.outcome = Outcome::Win;
    } else if (!alive0) {
        state.outcome = Outcome::Lose;
    }
    return state.outcome;
}

// Persona history model. Returns the guessed player type or NO_TYPE.
inline uint8_t predictPlayerType(const uint8_t *history, uint8_t historyCount,
                                 Persona persona, battle::Rng &rng)
{
    if (historyCount == 0) return NO_TYPE;
    if (persona == Persona::Reader)
        return history[historyCount - 1];

    // Frequency / anti-repeat: modal of counts; anti-repeat discounts the most
    // recent entry. Ties resolve uniformly through the seeded rng.
    uint8_t counts[TYPE_COUNT] = {0, 0, 0, 0, 0};
    uint8_t total = 0;
    for (uint8_t i = 0; i < historyCount; ++i) {
        const uint8_t type = history[i];
        if (type >= TYPE_COUNT) continue;
        const bool discount = persona == Persona::AntiRepeat &&
                              i == historyCount - 1;
        if (!discount) {
            ++counts[type];
            ++total;
        }
    }
    if (total == 0) return history[historyCount - 1];

    uint8_t bestCount = 0;
    for (uint8_t t = 0; t < TYPE_COUNT; ++t)
        if (counts[t] > bestCount) bestCount = counts[t];
    uint8_t tied[TYPE_COUNT];
    uint8_t tiedCount = 0;
    for (uint8_t t = 0; t < TYPE_COUNT; ++t)
        if (counts[t] == bestCount) tied[tiedCount++] = t;
    return tied[rng.roll(tiedCount)];
}

inline uint8_t benchCounterSlot(const State &state, uint8_t guessed)
{
    const Side &s = state.sides[1];
    for (uint8_t slot = 0; slot < s.count; ++slot) {
        if (slot == s.active || s.members[slot].hp == 0) continue;
        const Member &m = s.members[slot];
        for (uint8_t o = 0; o < m.optionCount; ++o)
            if (relation(m.options[o], guessed) == REL_WIN) return slot;
    }
    return NO_SLOT;
}

// Opponent pick. Reads cached public state and revealed history only; consumes
// no FX data and no hidden pick, so navigation consumes no extra RNG.
inline Action chooseAction(const State &state, Persona persona, uint8_t readRate,
                           battle::Rng &rng)
{
    const Member &active = state.sides[1].members[state.sides[1].active];
    if (active.optionCount == 0) return {ActionKind::Wait, NO_SLOT};

    const bool reads = persona != Persona::Random &&
                       rng.roll(100) < readRate;
    if (reads) {
        const uint8_t guessed =
            predictPlayerType(state.history, state.historyCount, persona, rng);
        if (guessed != NO_TYPE) {
            uint8_t counters[OPTION_CAP];
            uint8_t count = 0;
            for (uint8_t o = 0; o < active.optionCount; ++o)
                if (relation(active.options[o], guessed) == REL_WIN)
                    counters[count++] = o;
            if (count != 0)
                return {ActionKind::Attack, counters[rng.roll(count)]};
            const uint8_t bench = benchCounterSlot(state, guessed);
            if (bench != NO_SLOT) return {ActionKind::Switch, bench};
        }
        // A fired read with no counter falls back uniformly across every
        // attack slot; the matchup preference applies only when no read fires.
        return {ActionKind::Attack, rng.roll(active.optionCount)};
    }

    // No read: prefer the best creature-matchup tier so the AI never feeds a
    // weak pick, then fall back to uniform choice across attack slots.
    const Side &player = state.sides[0];
    const Member &target = player.members[player.active];
    uint8_t bestTier = 0;
    for (uint8_t o = 0; o < active.optionCount; ++o) {
        const uint8_t tier =
            creatureModifier(active.options[o], target.types, target.typeCount);
        if (tier > bestTier) bestTier = tier;
    }
    uint8_t pool[OPTION_CAP];
    uint8_t count = 0;
    for (uint8_t o = 0; o < active.optionCount; ++o)
        if (creatureModifier(active.options[o], target.types, target.typeCount) ==
            bestTier)
            pool[count++] = o;
    return {ActionKind::Attack, pool[rng.roll(count)]};
}

} // namespace duel
