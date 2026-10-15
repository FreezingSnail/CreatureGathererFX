#pragma once
// Permanent host proof for the compact duel menu pages (CreatureGathererFX-
// d73i.2): pick-row commit, WAIT (Pass), the SWAP bench-picker open/cancel, the
// wild-only action page, trainer suppression, and held-edge ownership.
#ifdef CGFX_DUEL_BATTLE

#include "test.hpp"
#include "../src/engine/battle/BattleMenu.hpp"
#include "../src/engine/battle/BattleState.hpp"
#include "../src/engine/menu/MenuV2.hpp"

namespace duel_menu_test_detail {

battle::BattleState oneAttackState()
{
    battle::BattleState state{};
    state.partyCount[0] = 3;
    state.activeSlot[0] = 0;
    state.bench[0][0].hp = 10;
    state.bench[0][1].hp = 10; // living bench -> the SWAP cell is offered
    state.active[0].moveIds[0] = 0;
    state.active[0].moveTypes[0] = static_cast<uint8_t>(Type::SPIRIT);
    // Zero-init leaves slots 1..3 with move id 0 (a valid authored id), so the
    // unused slots must be explicitly emptied or they count as attack cells.
    for (uint8_t slot = 1; slot < 4; ++slot) {
        state.active[0].moveTypes[slot] = EMPTY_MOVE_ID;
        state.active[0].moveIds[slot] = EMPTY_MOVE_ID;
    }
    return state;
}

battle::BattleState waitState()
{
    battle::BattleState state{};
    state.partyCount[0] = 3;
    state.activeSlot[0] = 0;
    state.bench[0][0].hp = 10;
    state.bench[0][1].hp = 10;
    for (uint8_t slot = 0; slot < 4; ++slot) {
        state.active[0].moveTypes[slot] = EMPTY_MOVE_ID;
        state.active[0].moveIds[slot] = EMPTY_MOVE_ID;
    }
    return state;
}

} // namespace duel_menu_test_detail

inline void DuelMenuSuite(TestSuite &suite)
{
    using namespace duel_menu_test_detail;
    Test test(__func__);

    // Pick row: A on an attack cell commits that original move slot.
    {
        battle::BattleMenu menu;
        menu.beginDuel(oneAttackState(), false, false);
        test.assert(menu.initialized(), true, "duel menu initializes the pick row");
        test.assert(menu.duelCursor(), static_cast<int8_t>(0), "cursor starts on the first cell");
        test.assert(menu.update(0).kind, MenuIntentKind::None, "a steady frame emits no intent");
        const MenuIntent intent = menu.update(MENU_EDGE_A);
        test.assert(intent.kind, MenuIntentKind::SelectMove, "A commits the attack cell");
        test.assert(intent.index, static_cast<uint8_t>(0), "the attack maps to its authored slot");
    }

    // SWAP opens the two-row picker; B cancels back to the swap cell.
    {
        battle::BattleMenu menu;
        menu.beginDuel(oneAttackState(), false, false);
        menu.update(MENU_NAV_RIGHT);
        test.assert(menu.duelCursor(), static_cast<int8_t>(1), "cursor reaches the SWAP cell");
        test.assert(menu.update(MENU_EDGE_A).kind, MenuIntentKind::None,
                    "SWAP opens the picker without an intent");
        test.assert(menu.partyPending(), true, "the picker page is pending");
        test.assert(menu.update(MENU_EDGE_B).kind, MenuIntentKind::None,
                    "B cancels the picker without an intent");
        test.assert(menu.duelCursor(), static_cast<int8_t>(1),
                    "cancel returns to the SWAP cell on the pick row");
    }

    // WAIT cell: an unusable kit offers SWAP + WAIT; A on WAIT emits Pass.
    {
        battle::BattleMenu menu;
        menu.beginDuel(waitState(), false, false);
        menu.update(MENU_NAV_RIGHT);
        test.assert(menu.duelCursor(), static_cast<int8_t>(1), "cursor reaches the WAIT cell");
        const MenuIntent intent = menu.update(MENU_EDGE_A);
        test.assert(intent.kind, MenuIntentKind::Pass, "WAIT commits Pass");
        test.assert(menu.update(MENU_EDGE_A).kind, MenuIntentKind::None,
                    "a held A does not re-send the WAIT intent");
    }

    // Wild action page: B opens it, A on the first row gathers, B returns.
    {
        battle::BattleMenu menu;
        menu.beginDuel(waitState(), true, true);
        test.assert(menu.update(MENU_EDGE_B).kind, MenuIntentKind::None,
                    "B opens the wild action page without an intent");
        test.assert(menu.update(MENU_EDGE_A).kind, MenuIntentKind::Gather,
                    "the first action row gathers");
        test.assert(menu.update(MENU_EDGE_B).kind, MenuIntentKind::None,
                    "B leaves the action page");
        test.assert(menu.duelCursor(), static_cast<int8_t>(0),
                    "the pick row is active again after the action page");
    }

    // A trainer (no gather/escape) suppresses the action page: B is inert.
    {
        battle::BattleMenu menu;
        menu.beginDuel(waitState(), false, false);
        menu.update(MENU_EDGE_B);
        test.assert(menu.update(MENU_EDGE_A).kind, MenuIntentKind::None,
                    "a trainer B leaves the pick row; A still acts on it");
        test.assert(menu.partyPending(), true,
                    "the trainer A on the SWAP cell opens the bench picker");
    }

    suite.addTest(test);
}

#endif // CGFX_DUEL_BATTLE
