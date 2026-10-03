#include "test.hpp"
#include "../src/engine/menu/MenuNav.hpp"

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

void MenuNavSuite(TestRunner &runner) {
    TestSuite suite("Menu navigation suite");
    MenuNavTest(suite);
    runner.addTestSuite(suite);
}
