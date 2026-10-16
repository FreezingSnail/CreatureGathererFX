#pragma once
// Compact duel view helpers (CreatureGathererFX-d73i.2). Pure, stateless reads
// of the resident BattleState used by both the session resolver and the duel
// renderer, so the pick row, centre, badges and feedback all derive from one
// source. No FX reads and no allocation; header-only under the duel flag.
#ifdef CGFX_DUEL_BATTLE

#include <stdint.h>
#include "BattleState.hpp"
#include "DuelKernel.hpp"

namespace duel {

// Feedback caption kinds, pinned to the mock/duel-bonus.html vocabulary plus the
// reused wild gather/escape/terminal captions. A terminal outcome always owns
// the caption so a win/lose/draw/flee/gather/escape is never hidden behind the
// round spelling; a progress-only gather keeps the in-progress "gather" caption.
enum class Feedback : uint8_t {
    None = 0,
    BonusWin,
    BonusLose,
    WeakWin,
    WeakLose,
    RpsWin,
    RpsLose,
    Trade,
    DefaultWin,
    Swap,
    Wait,
    Gather,    // gather round that neither acquired nor terminated
    Gathered,  // Outcome::Gathered
    Fled,      // Outcome::Fled
    Win,       // Outcome::Win
    Lose,      // Outcome::Lose
    Draw,      // Outcome::Draw (simultaneous KO)
    Escape,    // Outcome::Escaped (wild escape)
};

// Per-cell pick-row view. Cell order mirrors the mock: attack cells in ascending
// folded-type order (one per set mask bit), then SWAP (living bench), then WAIT
// (empty mask).
struct Cells {
    uint8_t count;      // total selectable cells (1..6)
    uint8_t options;    // usable attack cells (0..5)
    uint8_t swap;       // 1 when a living bench offers the SWAP cell
    uint8_t wait;       // 1 when the mask has no usable attack (WAIT cell)
    uint8_t round;      // 1-based round number
    uint8_t types[5];   // folded type per attack cell
    uint8_t markers[5]; // 0 none, 1 bonus (up), 2 weak (down)
    uint8_t slots[5];   // ascending option index per attack cell
};

// Folded active creature types for one side (1 or 2 distinct after folding).
inline uint8_t activeTypes(const battle::BattleState &state, uint8_t side,
                           uint8_t *out)
{
    const DualType types = state.active[side].types;
    const uint8_t first = static_cast<uint8_t>(types.getType1());
    const uint8_t second = static_cast<uint8_t>(types.getType2());
    out[0] = foldType(first);
    const uint8_t foldedSecond =
        second == static_cast<uint8_t>(Type::NONE) ? NO_TYPE : foldType(second);
    if (foldedSecond != NO_TYPE && foldedSecond != out[0]) {
        out[1] = foldedSecond;
        return 2;
    }
    return 1;
}

inline uint8_t usableOptionCount(const battle::BattleState &state, uint8_t side)
{
    return maskOptionCount(state.active[side].attackMask);
}

// Folded type of the option'th set mask bit (ascending type order).
inline uint8_t optionType(const battle::BattleState &state, uint8_t side,
                          uint8_t option)
{
    return maskOptionType(state.active[side].attackMask, option);
}

// True when any living member other than the active one exists (SWAP offered).
inline bool livingBench(const battle::BattleState &state, uint8_t side)
{
    const uint8_t count = state.partyCount[side] > PARTY_SIZE
        ? PARTY_SIZE : state.partyCount[side];
    const uint8_t active = state.activeSlot[side];
    for (uint8_t original = 0; original < count; ++original) {
        if (original == active) continue;
        const uint8_t bench = original < active ? original
                                                : static_cast<uint8_t>(original - 1);
        if (state.bench[side][bench].hp != 0) return true;
    }
    return false;
}

// Bonus marker for a cell: up (1) when the move's type beats any defender type,
// down (2) when it loses to one and beats none, else none (0).
inline uint8_t markerFor(uint8_t moveType, const uint8_t *defenderTypes,
                         uint8_t defenderCount)
{
    const uint8_t mod = creatureModifier(moveType, defenderTypes, defenderCount);
    return mod > MOD_SAME ? 1 : mod < MOD_SAME ? 2 : 0;
}

// Build the pick-row view for one side from resident state only.
inline void buildCells(const battle::BattleState &state, uint8_t side, Cells &out)
{
    out.options = usableOptionCount(state, side);
    out.swap = livingBench(state, side) ? 1 : 0;
    out.wait = out.options == 0 ? 1 : 0;
    out.round = state.round;
    out.count = static_cast<uint8_t>(out.options + out.swap + out.wait);
    if (out.count == 0) out.count = 1;

    uint8_t defenderTypes[2];
    const uint8_t defenderCount = activeTypes(state, side == 0 ? 1 : 0, defenderTypes);
    for (uint8_t option = 0; option < out.options; ++option) {
        const uint8_t type = optionType(state, side, option);
        out.types[option] = type;
        out.markers[option] = markerFor(type, defenderTypes, defenderCount);
        out.slots[option] = option;
    }
    for (uint8_t option = out.options; option < 5; ++option) {
        out.types[option] = NO_TYPE;
        out.markers[option] = 0;
        out.slots[option] = 255;
    }
}

// Caption for an attacking trade/win/lose round: the player's move relation
// against the opponent's move, labelled by the creature-type bonus on the side
// that lands the win (mock/duel-bonus.html drawDuelFeedback + duelResolve).
inline Feedback attackFeedback(uint8_t relation, uint8_t yourMod, uint8_t foeMod)
{
    if (relation == REL_WIN)
        return yourMod > MOD_SAME ? Feedback::BonusWin
             : yourMod < MOD_SAME ? Feedback::WeakWin : Feedback::RpsWin;
    if (relation == REL_LOSE)
        return foeMod > MOD_SAME ? Feedback::BonusLose
             : foeMod < MOD_SAME ? Feedback::WeakLose : Feedback::RpsLose;
    return Feedback::Trade;
}

} // namespace duel
#endif
