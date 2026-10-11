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
    menu.beginMoves(moves, true, false);
    test.assert(menu.initialized(), true,
                "beginMoves activates a prepared page");
    test.assert(FxReadCounter::count(), static_cast<uint8_t>(2),
                "transition caches only valid move names");
    FxReadCounter::resetFrame();
    MenuIntent moved = menu.update(NAV_RIGHT);
    test.assert(moved.kind, MenuIntentKind::None,
                "navigation only changes the move cursor");
    menu.draw();
    test.assert(FxReadCounter::count(), static_cast<uint8_t>(0),
                "cursor updates and drawing do not read metadata");
    MenuIntent selected = menu.update(EDGE_A);
    test.assert(selected.kind, MenuIntentKind::SelectMove,
                "A selects a usable move");
    test.assert(selected.index, static_cast<uint8_t>(2),
                "direction scan skips authored empty move IDs");
    test.assert(menu.update(0).kind, MenuIntentKind::None,
                "released input does not submit an action");
    menu.update(NAV_UP);
    test.assert(menu.update(EDGE_A).index, static_cast<uint8_t>(0),
                "vertical navigation uses the paired slot geometry");

    battle::MoveSnapshot empty = {};
    for (uint8_t slot = 0; slot < 4; ++slot) {
        empty.moveIds[slot] = EMPTY_MOVE_ID;
        empty.remainingUses[slot] = 0;
    }
    menu.beginMoves(empty, true, false);
    test.assert(menu.update(EDGE_A).kind, MenuIntentKind::Pass,
                "A submits Pass when every move is unavailable");
    test.assert(menu.update(EDGE_A).kind, MenuIntentKind::None,
                "Pass is emitted only once");
    menu.update(EDGE_B);
    menu.update(MENU_NAV_RIGHT);
    test.assert(menu.update(EDGE_A).kind, MenuIntentKind::None,
                "A does nothing for an unavailable Escape action");
    menu.update(MENU_NAV_LEFT);
    test.assert(menu.update(EDGE_A).kind, MenuIntentKind::Gather,
                "B opens actions and A submits an available Gather");
    test.assert(menu.update(EDGE_B).kind, MenuIntentKind::None,
                "B returns to moves without submitting an action");
    test.assert(menu.update(EDGE_A).kind, MenuIntentKind::Pass,
                "returning to moves leaves its page active");

    menu.beginMoves(empty, false, false);
    menu.update(EDGE_B);
    test.assert(menu.update(EDGE_A).kind, MenuIntentKind::None,
                "A does nothing when both direct actions are unavailable");
    menu.update(MENU_NAV_LEFT);
    test.assert(menu.update(EDGE_A).kind, MenuIntentKind::None,
                "the other unavailable action remains inert");
    test.assert(menu.update(EDGE_B).kind, MenuIntentKind::None,
                "B returns from actions without submitting a turn");

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
