#pragma once
// Permanent host proof for the compact duel view helpers (CreatureGathererFX-d73i.6).
// Compiled only in duel host runs: `make test CPPFLAGS='-I. -DCGFX_DUEL_BATTLE'`.
// Include this header under the same `#ifdef CGFX_DUEL_BATTLE` guard as the other
// duel suites, since DuelView.hpp is empty without the flag.
//
// Owned by CreatureGathererFX-d73i.6; no production/main changes. It asserts the
// current pure DuelView surface: attack-cell selection, original-slot mapping,
// folded active types + advantage-first markers, living-bench SWAP availability,
// the WAIT model and the caption classification.

#include "test.hpp"
#include "../src/engine/battle/DuelView.hpp"

namespace duel_view_contract_detail {

using duel::Cells;
using duel::Feedback;
// EMPTY_MOVE_ID (255) comes from src/lib/MoveIds.hpp, reached through the
// BattleState/DuelView include chain; a local alias would be ambiguous.

// A packed move carrying only an authored type (0..7 attack, 8 STATUS, 15 NONE).
inline Move attack(uint8_t authoredType)
{
    return Move(MoveBitSet{authoredType, 0, 0, 0, 0});
}

// Real setup clears every slot to the empty sentinel; Move() alone would read as
// authored SPIRIT, so both the packed move and the id must be set.
inline void emptySlots(battle::Combatant &combatant)
{
    for (uint8_t slot = 0; slot < 4; ++slot) {
        combatant.moves[slot] = Move();
        combatant.moveIds[slot] = EMPTY_MOVE_ID;
    }
}

inline void setSlot(battle::Combatant &combatant, uint8_t slot,
                    uint8_t authoredType, uint8_t moveId)
{
    combatant.moves[slot] = attack(authoredType);
    combatant.moveIds[slot] = moveId;
}

constexpr uint8_t authoredA = 1;
constexpr uint8_t authoredB = 2;
constexpr uint8_t authoredC = 3;

} // namespace duel_view_contract_detail

// Adds the DuelView contract test to `suite`. The caller owns registration into
// the TestRunner (same aggregation as the other `*Suite(TestSuite &)` helpers).
inline void DuelViewContractSuite(TestSuite &suite)
{
    using namespace duel_view_contract_detail;
    using battle::BattleState;

    Test test(__func__);

    // --- usableAttack filter: authored attack types only ---------------------
    test.assert(duel::usableAttack(attack(static_cast<uint8_t>(Type::SPIRIT))), true,
                "spirit move is usable");
    test.assert(duel::usableAttack(attack(static_cast<uint8_t>(Type::ELDER))), true,
                "elder move is usable");
    test.assert(duel::usableAttack(attack(static_cast<uint8_t>(Type::STATUS))), false,
                "STATUS move is not usable");
    test.assert(duel::usableAttack(attack(static_cast<uint8_t>(Type::NONE))), false,
                "NONE move is not usable");

    // --- four attacks: count, folded types, original-slot conversion ---------
    BattleState s = {};
    emptySlots(s.active[0]);
    setSlot(s.active[0], 0, static_cast<uint8_t>(Type::SPIRIT), authoredA);
    setSlot(s.active[0], 1, static_cast<uint8_t>(Type::WATER), authoredB);
    setSlot(s.active[0], 2, static_cast<uint8_t>(Type::WIND), authoredC);
    setSlot(s.active[0], 3, static_cast<uint8_t>(Type::FIRE), 4);
    test.assert(duel::usableOptionCount(s, 0), 4, "four attacks counted");
    test.assert(duel::slotForOption(s, 0, 0), 0, "option 0 -> original slot 0");
    test.assert(duel::slotForOption(s, 0, 3), 3, "option 3 -> original slot 3");
    test.assert(duel::optionType(s, 0, 0), 0, "spirit folds to spirit");
    test.assert(duel::optionType(s, 0, 2), 1, "wind folds to water");
    test.assert(duel::optionType(s, 0, 3), 3, "fire stays fire");

    // --- duplicate folded types are both kept, in slot order -----------------
    emptySlots(s.active[0]);
    setSlot(s.active[0], 0, static_cast<uint8_t>(Type::FIRE), authoredA);
    setSlot(s.active[0], 3, static_cast<uint8_t>(Type::FIRE), authoredB);
    test.assert(duel::usableOptionCount(s, 0), 2, "two duplicate-type attacks kept");
    test.assert(duel::optionType(s, 0, 0), 3, "first duplicate folds to fire");
    test.assert(duel::optionType(s, 0, 1), 3, "second duplicate folds to fire");
    test.assert(duel::slotForOption(s, 0, 1), 3, "second duplicate maps to slot 3");

    // --- STATUS / NONE / EMPTY exclusion -------------------------------------
    emptySlots(s.active[0]);
    setSlot(s.active[0], 0, static_cast<uint8_t>(Type::STATUS), authoredA); // status
    setSlot(s.active[0], 1, static_cast<uint8_t>(Type::FIRE), EMPTY_MOVE_ID); // empty id
    setSlot(s.active[0], 2, static_cast<uint8_t>(Type::NONE), authoredC);   // none
    setSlot(s.active[0], 3, static_cast<uint8_t>(Type::WIND), authoredB);   // usable
    test.assert(duel::usableOptionCount(s, 0), 1, "status/empty/none excluded");
    test.assert(duel::slotForOption(s, 0, 0), 3, "only slot 3 is usable");
    test.assert(duel::optionType(s, 0, 0), 1, "usable wind folds to water");

    // --- one attack ----------------------------------------------------------
    emptySlots(s.active[0]);
    setSlot(s.active[0], 2, static_cast<uint8_t>(Type::EARTH), authoredA);
    test.assert(duel::usableOptionCount(s, 0), 1, "single attack counted");
    test.assert(duel::slotForOption(s, 0, 0), 2, "single attack keeps original slot 2");
    test.assert(duel::optionType(s, 0, 0), 2, "earth stays earth");

    // --- zero attacks --------------------------------------------------------
    emptySlots(s.active[0]);
    test.assert(duel::usableOptionCount(s, 0), 0, "no usable attacks");
    test.assert(duel::slotForOption(s, 0, 0), 255, "no option has a slot");
    test.assert(duel::optionType(s, 0, 0), duel::NO_TYPE, "no option has a type");

    // --- species 0 is a real member, not an empty sentinel -------------------
    emptySlots(s.active[0]);
    s.active[0].id = 0;
    setSlot(s.active[0], 0, static_cast<uint8_t>(Type::WATER), 0); // move id 0 is real
    test.assert(s.active[0].id, 0, "species 0 active id valid");
    test.assert(duel::usableOptionCount(s, 0), 1, "move id 0 counts as a real attack");
    test.assert(duel::slotForOption(s, 0, 0), 0, "move id 0 maps to slot 0");

    // --- folded active types: dedup and distinct duals -----------------------
    uint8_t out[2] = {255, 255};
    s.active[1].types = DualType(Type::SPIRIT);
    test.assert(duel::activeTypes(s, 1, out), 1, "single spirit type");
    test.assert(out[0], 0, "spirit stays spirit");
    s.active[1].types = DualType(Type::WIND, Type::WATER);
    test.assert(duel::activeTypes(s, 1, out), 1, "wind+water dedup to one");
    test.assert(out[0], 1, "wind+water folds to water");
    s.active[1].types = DualType(Type::ELDER, Type::SPIRIT);
    test.assert(duel::activeTypes(s, 1, out), 1, "elder+spirit dedup to one");
    test.assert(out[0], 0, "elder+spirit folds to spirit");
    s.active[1].types = DualType(Type::FIRE, Type::LIGHTNING);
    test.assert(duel::activeTypes(s, 1, out), 1, "fire+lightning dedup to one");
    test.assert(out[0], 3, "fire+lightning folds to fire");
    s.active[1].types = DualType(Type::SPIRIT, Type::WATER);
    test.assert(duel::activeTypes(s, 1, out), 2, "spirit+water stays distinct");
    test.assert(out[0], 0, "distinct dual first type spirit");
    test.assert(out[1], 1, "distinct dual second type water");
    s.active[1].types = DualType(Type::EARTH, Type::WIND);
    test.assert(duel::activeTypes(s, 1, out), 2, "earth+wind stays distinct");
    test.assert(out[0], 2, "earth folds to earth");
    test.assert(out[1], 1, "wind folds to water");

    // --- advantage-first arrow markers ---------------------------------------
    const uint8_t fireWater[2] = {3, 1};
    test.assert(duel::markerFor(4, fireWater, 2), 1,
                "advantage-first: plant beats water, ignores the fire loss");
    const uint8_t fireOnly[1] = {3};
    test.assert(duel::markerFor(4, fireOnly, 1), 2, "plant-only loses to fire");
    test.assert(duel::markerFor(1, fireOnly, 1), 1, "water beats fire");
    const uint8_t waterOnly[1] = {1};
    test.assert(duel::markerFor(1, waterOnly, 1), 0, "mirror is a neutral marker");

    // --- living bench -> SWAP availability -----------------------------------
    BattleState b = {};
    b.partyCount[0] = 1;
    b.activeSlot[0] = 0;
    test.assert(duel::livingBench(b, 0), false, "one member offers no swap");

    b.partyCount[0] = 3;
    b.activeSlot[0] = 0;
    b.bench[0][0] = {};
    b.bench[0][0].id = 0; // species 0 is alive, not empty
    b.bench[0][0].hp = 7;
    b.bench[0][1] = {};
    b.bench[0][1].hp = 0;
    test.assert(duel::livingBench(b, 0), true, "species 0 living bench offers swap");

    b.bench[0][0].hp = 0;
    test.assert(duel::livingBench(b, 0), false, "all-KO bench suppresses swap");

    b.partyCount[0] = 2;
    b.activeSlot[0] = 0;
    b.bench[0][0] = {};
    b.bench[0][0].hp = 9;
    test.assert(duel::livingBench(b, 0), true, "two members with living bench swap");
    b.bench[0][0].hp = 0;
    test.assert(duel::livingBench(b, 0), false, "two members empty bench suppresses swap");

    // --- buildCells: four attacks, no bench, defender water ------------------
    BattleState cells = {};
    emptySlots(cells.active[0]);
    setSlot(cells.active[0], 0, static_cast<uint8_t>(Type::FIRE), authoredA);
    setSlot(cells.active[0], 1, static_cast<uint8_t>(Type::FIRE), authoredB);
    setSlot(cells.active[0], 2, static_cast<uint8_t>(Type::WIND), EMPTY_MOVE_ID);
    setSlot(cells.active[0], 3, static_cast<uint8_t>(Type::STATUS), authoredC);
    cells.partyCount[0] = 1;
    cells.activeSlot[0] = 0;
    cells.active[0].types = DualType(Type::FIRE);
    cells.round = 3;
    cells.active[1].types = DualType(Type::WATER);
    Cells view = {};
    duel::buildCells(cells, 0, view);
    test.assert(view.options, 2, "buildCells counts the two usable attacks");
    test.assert(view.swap, 0, "no living bench suppresses SWAP");
    test.assert(view.wait, 0, "usable kit has no WAIT cell");
    test.assert(view.count, 2, "two attack cells only");
    test.assert(view.round, 3, "round number passes through");
    test.assert(view.types[0], 3, "cell 0 fire");
    test.assert(view.types[1], 3, "cell 1 duplicate fire");
    test.assert(view.slots[0], 0, "cell 0 original slot 0");
    test.assert(view.slots[1], 1, "cell 1 original slot 1");
    test.assert(view.markers[0], 2, "fire Vs water is a weak marker");
    test.assert(view.markers[1], 2, "duplicate fire keeps the weak marker");
    test.assert(view.types[2], duel::NO_TYPE, "unused cell has no type");
    test.assert(view.markers[2], 0, "unused cell has no marker");
    test.assert(view.slots[2], 255, "unused cell has no slot");

    // Same kit with one living bench: SWAP cell appended after the attacks.
    cells.partyCount[0] = 2;
    cells.bench[0][0] = {};
    cells.bench[0][0].hp = 4;
    duel::buildCells(cells, 0, view);
    test.assert(view.swap, 1, "living bench appends SWAP");
    test.assert(view.count, 3, "two attacks plus SWAP");

    // --- buildCells: unusable kit -> WAIT (SWAP only when a bench lives) -----
    BattleState waitState = {};
    emptySlots(waitState.active[0]);
    waitState.partyCount[0] = 1;
    waitState.activeSlot[0] = 0;
    duel::buildCells(waitState, 0, view);
    test.assert(view.options, 0, "unusable kit has no attack cells");
    test.assert(view.wait, 1, "unusable kit shows WAIT");
    test.assert(view.swap, 0, "unusable kit without bench has no SWAP");
    test.assert(view.count, 1, "WAIT is the only cell");

    waitState.partyCount[0] = 2;
    waitState.bench[0][0] = {};
    waitState.bench[0][0].hp = 4;
    duel::buildCells(waitState, 0, view);
    test.assert(view.wait, 1, "unusable kit with bench still shows WAIT");
    test.assert(view.swap, 1, "unusable kit with living bench shows SWAP");
    test.assert(view.count, 2, "WAIT plus SWAP cells");

    // --- caption classification ----------------------------------------------
    test.assert(duel::attackFeedback(duel::REL_WIN, duel::MOD_BONUS, duel::MOD_SAME),
                Feedback::BonusWin, "bonus win");
    test.assert(duel::attackFeedback(duel::REL_WIN, duel::MOD_HALF, duel::MOD_SAME),
                Feedback::WeakWin, "weak win");
    test.assert(duel::attackFeedback(duel::REL_WIN, duel::MOD_SAME, duel::MOD_BONUS),
                Feedback::RpsWin, "rps win");
    test.assert(duel::attackFeedback(duel::REL_LOSE, duel::MOD_SAME, duel::MOD_BONUS),
                Feedback::BonusLose, "bonus lose");
    test.assert(duel::attackFeedback(duel::REL_LOSE, duel::MOD_SAME, duel::MOD_HALF),
                Feedback::WeakLose, "weak lose");
    test.assert(duel::attackFeedback(duel::REL_LOSE, duel::MOD_BONUS, duel::MOD_SAME),
                Feedback::RpsLose, "rps lose");
    test.assert(duel::attackFeedback(duel::REL_TRADE, duel::MOD_HALF, duel::MOD_BONUS),
                Feedback::Trade, "trade");

    suite.addTest(test);
}
