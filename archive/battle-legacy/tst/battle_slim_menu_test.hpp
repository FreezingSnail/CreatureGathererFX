#pragma once

#include "test.hpp"
#include "../src/engine/battle/BattleMenu.hpp"
#include "../src/engine/menu/MenuNav.hpp"
#include "../src/lib/FxReadCounter.hpp"
#include "../src/lib/MoveIds.hpp"

inline void BattleSlimMenuTest(TestSuite &suite)
{
    Test test(__func__);
    constexpr uint8_t EDGE_A = 1u << 3;
    constexpr uint8_t EDGE_B = 1u << 2;
    constexpr uint8_t NAV_RIGHT = MENU_NAV_RIGHT;
    constexpr uint8_t NAV_LEFT = MENU_NAV_LEFT;
    constexpr uint8_t NAV_UP = MENU_NAV_UP;
    constexpr uint8_t NAV_DOWN = MENU_NAV_DOWN;

    battle::MoveSnapshot moves = {};
    for (uint8_t slot = 0; slot < 4; ++slot) {
        moves.moveIds[slot] = EMPTY_MOVE_ID;
        moves.remainingUses[slot] = 0;
    }
    moves.moveIds[0] = 0;
    moves.remainingUses[0] = 3;
    moves.moveIds[1] = LEGACY_EMPTY_MOVE_ID;
    moves.remainingUses[1] = 3;
    moves.moveIds[2] = 1;
    moves.remainingUses[2] = 2;
    moves.moveIds[3] = 35;
    moves.remainingUses[3] = 3;

    battle::BattleMenu menu;
    test.assert(menu.initialized(), false,
                "default menu has no prepared choice page");
    FxReadCounter::resetFrame();
    menu.beginOptions(moves, true, false);
    test.assert(menu.initialized(), true,
                "beginOptions activates the options root page");
    test.assert(FxReadCounter::count(), static_cast<uint8_t>(2),
                "transition caches only valid move names");
    FxReadCounter::resetFrame();
    menu.update(NAV_DOWN);
    test.assert(menu.update(NAV_UP).kind, MenuIntentKind::None,
                "options navigation only moves the cursor");
    menu.draw();
    test.assert(FxReadCounter::count(), static_cast<uint8_t>(0),
                "cursor updates and drawing do not read metadata");

    // Options grid: MOVE(0) GATHER(1) / CHANGE(2) ESCAPE(3).
    menu.update(NAV_RIGHT);
    test.assert(menu.update(EDGE_A).kind, MenuIntentKind::Gather,
                "A on an available Gather submits the action");
    menu.update(NAV_DOWN);
    test.assert(menu.update(EDGE_A).kind, MenuIntentKind::None,
                "A does nothing for an unavailable Escape action");
    menu.update(NAV_LEFT);
    menu.update(EDGE_A);
    test.assert(menu.partyPending(), true,
                "CHANGE opens the pending party page");
    battle::BattleView view = {};
    view.partyCount[0] = 3;
    view.activeSlot[0] = 0;
    view.party[0][0] = {0, 10, 1};
    view.party[0][1] = {1, 12, 1};
    view.party[0][2] = {2, 14, 1};
    FxReadCounter::resetFrame();
    menu.beginParty(view);
    test.assert(FxReadCounter::count(), static_cast<uint8_t>(2),
                "party transition caches both living bench names");
    FxReadCounter::resetFrame();
    menu.update(NAV_DOWN);
    test.assert(menu.update(NAV_UP).kind, MenuIntentKind::None,
                "party rows only move the cursor");
    const MenuIntent firstParty = menu.update(EDGE_A);
    test.assert(firstParty.kind, MenuIntentKind::SelectParty,
                "A on a party row emits SelectParty");
    test.assert(firstParty.index, static_cast<uint8_t>(1),
                "first visible row keeps its original bench slot");
    test.assert(menu.partyPending(), false,
                "selecting a row leaves the options root");

    // MOVE opens the four-slot page; B returns to the options root.
    menu.update(EDGE_A);
    const MenuIntent moved = menu.update(NAV_RIGHT);
    test.assert(moved.kind, MenuIntentKind::None,
                "navigation only changes the move cursor");
    const MenuIntent selected = menu.update(EDGE_A);
    test.assert(selected.kind, MenuIntentKind::SelectMove,
                "A selects a usable move");
    test.assert(selected.index, static_cast<uint8_t>(2),
                "direction scan skips authored empty move IDs");
    test.assert(menu.update(0).kind, MenuIntentKind::None,
                "released input does not submit an action");
    menu.update(NAV_UP);
    test.assert(menu.update(EDGE_A).index, static_cast<uint8_t>(0),
                "vertical navigation uses the paired slot geometry");
    test.assert(menu.update(EDGE_B).kind, MenuIntentKind::None,
                "B returns to the options root");
    test.assert(menu.update(EDGE_A).kind, MenuIntentKind::None,
                "A reopens the moves page from the root");

    battle::MoveSnapshot empty = {};
    for (uint8_t slot = 0; slot < 4; ++slot) {
        empty.moveIds[slot] = EMPTY_MOVE_ID;
        empty.remainingUses[slot] = 0;
    }
    menu.beginOptions(empty, true, false);
    menu.update(EDGE_A);
    test.assert(menu.update(EDGE_A).kind, MenuIntentKind::Pass,
                "A submits Pass when every move is unavailable");
    test.assert(menu.update(EDGE_A).kind, MenuIntentKind::None,
                "Pass is emitted only once");
    menu.update(EDGE_B);
    menu.update(NAV_DOWN);
    menu.update(NAV_RIGHT);
    test.assert(menu.update(EDGE_A).kind, MenuIntentKind::None,
                "A does nothing for an unavailable Escape action");
    menu.update(NAV_LEFT);
    menu.update(NAV_UP);
    menu.update(NAV_RIGHT);
    test.assert(menu.update(EDGE_A).kind, MenuIntentKind::Gather,
                "Gather remains available from the options root");
    menu.update(NAV_LEFT);
    menu.update(EDGE_A);
    test.assert(menu.update(EDGE_A).kind, MenuIntentKind::Pass,
                "returning to moves leaves its page active");

    menu.beginOptions(empty, false, false);
    menu.update(NAV_RIGHT);
    test.assert(menu.update(EDGE_A).kind, MenuIntentKind::None,
                "A does nothing when both direct actions are unavailable");
    menu.update(NAV_DOWN);
    test.assert(menu.update(EDGE_A).kind, MenuIntentKind::None,
                "the other unavailable action remains inert");

#if !CGFX_BATTLE_AUTO_REPLACE
    battle::ReplacementChoices party = {};
    party.count = 2;
    party.ids[0] = 0;
    party.slots[0] = 0;
    party.ids[1] = 1;
    party.slots[1] = 2;
    FxReadCounter::resetFrame();
    menu.beginReplacement(party);
    test.assert(FxReadCounter::count(), static_cast<uint8_t>(2),
                "replacement transition caches both living names");
    FxReadCounter::resetFrame();
    menu.update(NAV_DOWN);
    menu.draw();
    test.assert(FxReadCounter::count(), static_cast<uint8_t>(0),
                "replacement cursor updates and drawing do not read metadata");
    const MenuIntent replacement = menu.update(EDGE_A);
    test.assert(replacement.kind, MenuIntentKind::SelectParty,
                "choice mode accepts a living replacement");
    test.assert(replacement.index, static_cast<uint8_t>(2),
                "replacement submits its original party slot");
    test.assert(menu.update(EDGE_B).kind, MenuIntentKind::None,
                "B has no effect during replacement");

    party.count = 1;
    party.ids[0] = 0;
    party.slots[0] = 1;
    menu.beginReplacement(party);
    menu.update(NAV_UP);
    const MenuIntent sole = menu.update(EDGE_A);
    test.assert(sole.kind, MenuIntentKind::SelectParty,
                "a sole living bench row remains selectable");
    test.assert(sole.index, static_cast<uint8_t>(1),
                "a single visible row keeps its original party slot");

    party.ids[0] = 255;
    FxReadCounter::resetFrame();
    menu.beginReplacement(party);
    test.assert(FxReadCounter::count(), static_cast<uint8_t>(0),
                "sentinel species does not trigger name metadata reads");
    test.assert(menu.update(EDGE_A).kind, MenuIntentKind::None,
                "sentinel replacement row cannot submit player input");
#endif
    suite.addTest(test);
}

inline void BattleSlimMenuSuite(TestRunner &runner)
{
    TestSuite suite("Slim battle menu pages");
    BattleSlimMenuTest(suite);
    runner.addTestSuite(suite);
}
