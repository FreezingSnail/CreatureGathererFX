#pragma once
// Permanent host proof for the compact duel view helpers (CreatureGathererFX-d73i.6,
// reworked to the 5-bit attack mask by CreatureGathererFX-4w89.3).
// Compiled only in duel host runs: `make test CPPFLAGS='-I. -DCGFX_DUEL_BATTLE'`.
// Include this header under the same `#ifdef CGFX_DUEL_BATTLE` guard as the other
// duel suites, since DuelView.hpp is empty without the flag.
//
// It asserts the current pure DuelView surface: option enumeration from the
// active attack mask (ascending folded-type order), folded active types +
// advantage-first markers, living-bench SWAP availability, the WAIT model and
// the caption classification.

#include "test.hpp"
#include "../src/engine/battle/DuelView.hpp"

namespace duel_view_contract_detail {

using duel::Cells;
using duel::Feedback;

// A single folded mask bit, as built by BattleSetup's duelAttackMask fold.
inline uint8_t bitFor(Type authored)
{
    return static_cast<uint8_t>(1u << duel::foldType(static_cast<uint8_t>(authored)));
}

constexpr uint8_t spiritBit = 1u << 0;
constexpr uint8_t waterBit = 1u << 1;
constexpr uint8_t earthBit = 1u << 2;
constexpr uint8_t fireBit = 1u << 3;
constexpr uint8_t plantBit = 1u << 4;

} // namespace duel_view_contract_detail

// Adds the DuelView contract test to `suite`. The caller owns registration into
// the TestRunner (same aggregation as the other `*Suite(TestSuite &)` helpers).
inline void DuelViewContractSuite(TestSuite &suite)
{
    using namespace duel_view_contract_detail;
    using battle::BattleState;

    Test test(__func__);

    // --- fold map sanity: authored 8-type -> 5 duel bit ----------------------
    test.assert(bitFor(Type::SPIRIT), spiritBit, "spirit folds to bit 0");
    test.assert(bitFor(Type::WIND), waterBit, "wind folds to water bit");
    test.assert(bitFor(Type::LIGHTNING), fireBit, "lightning folds to fire bit");
    test.assert(bitFor(Type::ELDER), spiritBit, "elder folds to spirit bit");

    // --- four attacks: count, ascending folded types, option slots -----------
    BattleState s = {};
    s.active[0].attackMask = static_cast<uint8_t>(spiritBit | waterBit | earthBit | fireBit);
    test.assert(duel::usableOptionCount(s, 0), 4, "four mask bits counted");
    test.assert(duel::optionType(s, 0, 0), 0, "option 0 is spirit");
    test.assert(duel::optionType(s, 0, 1), 1, "option 1 is water");
    test.assert(duel::optionType(s, 0, 2), 2, "option 2 is earth");
    test.assert(duel::optionType(s, 0, 3), 3, "option 3 is fire");
    test.assert(duel::optionType(s, 0, 4), duel::NO_TYPE, "option 4 is out of range");

    // --- sparse mask enumerates set bits in ascending type order -------------
    s.active[0].attackMask = static_cast<uint8_t>(plantBit | waterBit);
    test.assert(duel::usableOptionCount(s, 0), 2, "two set bits counted");
    test.assert(duel::optionType(s, 0, 0), 1, "lowest bit (water) is option 0");
    test.assert(duel::optionType(s, 0, 1), 4, "highest bit (plant) is option 1");
    test.assert(duel::optionType(s, 0, 2), duel::NO_TYPE, "past the last bit is NO_TYPE");

    // --- a duplicate authored type folds to one bit (mask dedupes) -----------
    s.active[0].attackMask = static_cast<uint8_t>(fireBit);
    test.assert(duel::usableOptionCount(s, 0), 1, "fire+fire folds to one bit");
    test.assert(duel::optionType(s, 0, 0), 3, "the single bit is fire");

    // --- full five-type mask ------------------------------------------------
    s.active[0].attackMask = 0x1F;
    test.assert(duel::usableOptionCount(s, 0), 5, "all five duel types are options");
    test.assert(duel::optionType(s, 0, 4), 4, "the fifth option is plant");

    // --- zero mask ----------------------------------------------------------
    s.active[0].attackMask = 0;
    test.assert(duel::usableOptionCount(s, 0), 0, "no usable attacks");
    test.assert(duel::optionType(s, 0, 0), duel::NO_TYPE, "no option has a type");

    // --- species 0 is a real member, not an empty sentinel -------------------
    s.active[0].id = 0;
    s.active[0].attackMask = waterBit;
    test.assert(s.active[0].id, 0, "species 0 active id valid");
    test.assert(duel::usableOptionCount(s, 0), 1, "species 0 mask is a real attack");
    test.assert(duel::optionType(s, 0, 0), 1, "species 0 water bit");

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

    // --- buildCells: two set bits (water+fire), no bench ---------------------
    BattleState cells = {};
    cells.active[0].attackMask = static_cast<uint8_t>(fireBit | waterBit);
    cells.partyCount[0] = 1;
    cells.activeSlot[0] = 0;
    cells.active[0].types = DualType(Type::FIRE);
    cells.round = 3;
    cells.active[1].types = DualType(Type::WATER);
    Cells view = {};
    duel::buildCells(cells, 0, view);
    test.assert(view.options, 2, "buildCells counts the two set bits");
    test.assert(view.swap, 0, "no living bench suppresses SWAP");
    test.assert(view.wait, 0, "usable mask has no WAIT cell");
    test.assert(view.count, 2, "two attack cells only");
    test.assert(view.round, 3, "round number passes through");
    test.assert(view.types[0], 1, "cell 0 water (lowest bit)");
    test.assert(view.types[1], 3, "cell 1 fire");
    test.assert(view.slots[0], 0, "cell 0 option index 0");
    test.assert(view.slots[1], 1, "cell 1 option index 1");
    test.assert(view.markers[0], 0, "water Vs water is neutral");
    test.assert(view.markers[1], 2, "fire Vs water is a weak marker");
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

    // --- buildCells: empty mask -> WAIT (SWAP only when a bench lives) ------
    BattleState waitState = {};
    waitState.active[0].attackMask = 0;
    waitState.partyCount[0] = 1;
    waitState.activeSlot[0] = 0;
    duel::buildCells(waitState, 0, view);
    test.assert(view.options, 0, "empty mask has no attack cells");
    test.assert(view.wait, 1, "empty mask shows WAIT");
    test.assert(view.swap, 0, "empty mask without bench has no SWAP");
    test.assert(view.count, 1, "WAIT is the only cell");

    waitState.partyCount[0] = 2;
    waitState.bench[0][0] = {};
    waitState.bench[0][0].hp = 4;
    duel::buildCells(waitState, 0, view);
    test.assert(view.wait, 1, "empty mask with bench still shows WAIT");
    test.assert(view.swap, 1, "empty mask with living bench shows SWAP");
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
