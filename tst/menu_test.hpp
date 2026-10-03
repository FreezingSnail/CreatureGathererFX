#include "test.hpp"
#include "../src/engine/menu/MenuNav.hpp"
#include "../src/engine/menu/MenuV2.hpp"
#include "../src/lib/FxReadCounter.hpp"

void MenuNavTest(TestSuite &suite) {
    Test test(__func__);

    const MenuEnum menus[] = {
        BATTLE_OPTIONS,
        BATTLE_MOVE_SELECT,
        BATTLE_CREATURE_SELECT,
    };
    const uint8_t expectedLeft[] = {3, 0, 1, 2};
    const uint8_t expectedRight[] = {1, 2, 3, 0};
    const uint8_t expectedUp[] = {3, 3, 0, 1};
    const uint8_t expectedDown[] = {2, 3, 0, 0};

    for (MenuEnum menu : menus) {
        const MenuDesc &desc = menuDescFor(menu);
        test.assert(desc.itemCount, static_cast<uint8_t>(4), "battle menu item count");
        test.assert(desc.cols, static_cast<uint8_t>(2), "battle menu column count");
        test.assert(desc.wrap, static_cast<uint8_t>(1), "battle menu wraps");
        for (uint8_t cursor = 0; cursor < 4; ++cursor) {
            test.assert(menuNavMove(desc, cursor, MENU_NAV_LEFT), expectedLeft[cursor],
                         "left movement matches 2x2 table");
            test.assert(menuNavMove(desc, cursor, MENU_NAV_RIGHT), expectedRight[cursor],
                         "right movement matches 2x2 table");
            test.assert(menuNavMove(desc, cursor, MENU_NAV_UP), expectedUp[cursor],
                         "up movement matches 2x2 table");
            test.assert(menuNavMove(desc, cursor, MENU_NAV_DOWN), expectedDown[cursor],
                         "down movement matches 2x2 table");
        }
    }

    const MenuDesc &arena = menuDescFor(ARENA_MENU);
    test.assert(arena.itemCount, static_cast<uint8_t>(31), "arena item count");
    test.assert(arena.cols, static_cast<uint8_t>(1), "arena column count");
    test.assert(arena.wrap, static_cast<uint8_t>(0), "arena does not wrap");
    test.assert(menuNavMove(arena, 0, MENU_NAV_LEFT), static_cast<uint8_t>(0),
                 "arena left edge clamps");
    test.assert(menuNavMove(arena, 0, MENU_NAV_UP), static_cast<uint8_t>(0),
                 "arena upper edge clamps");
    test.assert(menuNavMove(arena, 30, MENU_NAV_RIGHT), static_cast<uint8_t>(30),
                 "arena right edge does not wrap");
    test.assert(menuNavMove(arena, 30, MENU_NAV_DOWN), static_cast<uint8_t>(30),
                 "arena lower edge does not wrap");
    test.assert(menuNavMove(arena, 5, MENU_NAV_DOWN), static_cast<uint8_t>(6),
                 "arena moves by one row");

    const MenuDesc one = {1, 2, 1};
    test.assert(menuNavMove(one, 0, MENU_NAV_LEFT | MENU_NAV_RIGHT | MENU_NAV_UP |
                                      MENU_NAV_DOWN),
                static_cast<uint8_t>(0), "single-item menu never leaves zero");
    test.assert(menuNavMove(one, 200, 0), static_cast<uint8_t>(0),
                "single-item cursor is bounded");

    suite.addTest(test);
}

void MenuIntentTest(TestSuite &suite);

void MenuNavSuite(TestRunner &runner) {
    TestSuite suite("Menu navigation suite");
    MenuNavTest(suite);
    MenuIntentTest(suite);
    runner.addTestSuite(suite);
}


void MenuIntentTest(TestSuite &suite) {
    Test test(__func__);
    MenuV2 menu;

    const auto openRoot = [&menu]() {
        menu.clear();
        menu.push(BATTLE_OPTIONS);
    };
    const auto assertNone = [&test](const MenuIntent &intent,
                                    const char *message) {
        test.assert(intent.kind, MenuIntentKind::None, message);
    };

    assertNone(menu.update(MENU_EDGE_A), "closed menu ignores A");
    test.assert(menu.menuPointer, static_cast<int8_t>(-1),
                "closed menu retains ownership boundary");

    openRoot();
    menu.cursorIndex = 0;
    MenuIntent intent = menu.update(MENU_EDGE_A);
    assertNone(intent, "root A opens move submenu without fallthrough");
    test.assert(menu.menuPointer, static_cast<int8_t>(1),
                "move submenu owned by menu");
    test.assert(menu.stack[1], BATTLE_MOVE_SELECT,
                "root option zero opens moves");

    for (uint8_t move = 0; move < 4; ++move) {
        openRoot();
        menu.cursorIndex = 0;
        assertNone(menu.update(MENU_EDGE_A), "move submenu opens for every move");
        menu.cursorIndex = static_cast<int8_t>(move);
        intent = menu.update(MENU_EDGE_A);
        test.assert(intent.kind, MenuIntentKind::SelectMove,
                    "move row emits SelectMove");
        test.assert(intent.index, move, "move row preserves move slot");
        test.assert(menu.menuPointer, static_cast<int8_t>(0),
                    "move selection returns to root owner");
    }

    openRoot();
    menu.cursorIndex = 1;
    intent = menu.update(MENU_EDGE_A);
    test.assert(intent.kind, MenuIntentKind::Gather,
                "root option one emits Gather");
    test.assert(intent.index, static_cast<uint8_t>(0),
                "Gather has neutral index");
    test.assert(menu.menuPointer, static_cast<int8_t>(0),
                "Gather leaves root ownership with menu");

    openRoot();
    menu.cursorIndex = 3;
    intent = menu.update(MENU_EDGE_A);
    test.assert(intent.kind, MenuIntentKind::Escape,
                "root option three emits Escape");
    test.assert(intent.index, static_cast<uint8_t>(0),
                "Escape has neutral index");

    openRoot();
    menu.cursorIndex = 2;
    assertNone(menu.update(MENU_EDGE_A), "root option two opens party submenu");
    intent = menu.update(MENU_EDGE_B);
    test.assert(intent.kind, MenuIntentKind::Back,
                "B backs out of voluntary party submenu");
    test.assert(menu.menuPointer, static_cast<int8_t>(0),
                "voluntary Back pops only submenu");

    battle::PartySnapshot snapshot = {};
    snapshot.choices[0] = {0, 2, 40}; // species zero is valid, original slot two
    snapshot.choices[1] = {7, 0, 12};
    snapshot.count = 2;
    snapshot.forced = 0;
    menu.setPartySnapshot(snapshot);
    test.assert(menu.partySnapshot().choices[0].slot, static_cast<uint8_t>(2),
                "party snapshot keeps original slot");
    test.assert(menu.partySnapshot().choices[0].hp, static_cast<uint8_t>(40),
                "party snapshot remains copied at open boundary");

    openRoot();
    menu.setPartySnapshot(snapshot);
    menu.cursorIndex = 2;
    assertNone(menu.update(MENU_EDGE_A), "party submenu opens from root option two");
    test.assert(menu.cursorIndex, static_cast<int8_t>(0),
                "party submenu starts at first row");
    assertNone(menu.update(MENU_NAV_DOWN), "party navigation does not emit intent");
    test.assert(menu.cursorIndex, static_cast<int8_t>(1),
                "party navigation uses rows, not cursor divided by two");
    intent = menu.update(MENU_EDGE_A);
    test.assert(intent.kind, MenuIntentKind::SelectParty,
                "party row emits SelectParty");
    test.assert(intent.index, static_cast<uint8_t>(0),
                "party row emits selected original slot");
    test.assert(menu.menuPointer, static_cast<int8_t>(0),
                "party selection returns to root owner");

    snapshot.choices[0].hp = 0;
    snapshot.choices[1].slot = 3;
    snapshot.choices[1].hp = 12;
    menu.setPartySnapshot(snapshot);
    openRoot();
    menu.setPartySnapshot(snapshot);
    menu.cursorIndex = 2;
    assertNone(menu.update(MENU_EDGE_A), "invalid-choice party submenu opens");
    menu.cursorIndex = 0;
    assertNone(menu.update(MENU_EDGE_A), "dead party row is rejected");
    test.assert(menu.menuPointer, static_cast<int8_t>(1),
                "dead choice keeps party menu owned");
    menu.cursorIndex = 1;
    assertNone(menu.update(MENU_EDGE_A), "out-of-range original slot is rejected");
    test.assert(menu.menuPointer, static_cast<int8_t>(1),
                "invalid choice keeps party menu owned");

    snapshot.choices[0] = {0, 2, 0};
    snapshot.choices[1] = {9, 1, 8};
    snapshot.count = 2;
    snapshot.forced = 1;
    menu.setPartySnapshot(snapshot);
    openRoot();
    menu.setPartySnapshot(snapshot);
    menu.cursorIndex = 2;
    assertNone(menu.update(MENU_EDGE_A), "forced party submenu opens");
    intent = menu.update(MENU_EDGE_B);
    assertNone(intent, "forced replacement ignores B");
    test.assert(menu.menuPointer, static_cast<int8_t>(1),
                "forced B does not surrender menu ownership");
    menu.cursorIndex = 0;
    assertNone(menu.update(MENU_EDGE_A), "forced replacement rejects dead row");
    test.assert(menu.menuPointer, static_cast<int8_t>(1),
                "forced dead choice keeps menu open");
    menu.cursorIndex = 1;
    intent = menu.update(MENU_EDGE_A);
    test.assert(intent.kind, MenuIntentKind::SelectParty,
                "forced replacement accepts live row");
    test.assert(intent.index, static_cast<uint8_t>(1),
                "forced replacement preserves original slot");

    openRoot();
    intent = menu.update(MENU_EDGE_B);
    assertNone(intent, "B on root is ignored");
    test.assert(menu.menuPointer, static_cast<int8_t>(0),
                "root B keeps menu ownership");

    FxReadCounter::resetFrame();
    assertNone(menu.update(0), "idle update emits no intent");
    test.assert(FxReadCounter::count(), static_cast<uint8_t>(0),
                "pure menu update performs no FX reads");

    suite.addTest(test);
}
