#include "test.hpp"
#include "../src/engine/menu/MenuNav.hpp"
#include "../src/engine/menu/MenuV2.hpp"
#include "../src/engine/menu/PackedMoveInfo.hpp"
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
void MenuSnapshotIntegrationTest(TestSuite &suite);

static void writeMoveInfoOracle(uint8_t *packed, uint8_t slot, uint16_t info) {
    for (uint8_t bit = 0; bit < 10; ++bit) {
        const uint8_t offset = static_cast<uint8_t>(slot * 10 + bit);
        const uint8_t mask = static_cast<uint8_t>(1u << (offset & 7));
        if (info & (1u << bit)) packed[offset >> 3] |= mask;
        else packed[offset >> 3] &= static_cast<uint8_t>(~mask);
    }
}

static uint16_t readMoveInfoOracle(const uint8_t *packed, uint8_t slot) {
    uint16_t info = 0;
    for (uint8_t bit = 0; bit < 10; ++bit) {
        const uint8_t offset = static_cast<uint8_t>(slot * 10 + bit);
        if (packed[offset >> 3] & (1u << (offset & 7))) info |= 1u << bit;
    }
    return info;
}

void PackedMoveInfoCodecTest(TestSuite &suite) {
    Test test(__func__);
    uint8_t actual[PackedMoveInfo::BYTE_COUNT] = {0xa5, 0x5a, 0xc3, 0x3c, 0x96};
    uint8_t expected[PackedMoveInfo::BYTE_COUNT];
    for (uint8_t byte = 0; byte < PackedMoveInfo::BYTE_COUNT; ++byte) {
        expected[byte] = actual[byte];
    }

    for (uint8_t slot = 0; slot < PackedMoveInfo::SLOT_COUNT; ++slot) {
        for (uint16_t info = 0; info < 1024; ++info) {
            PackedMoveInfo::write(actual, slot, info);
            writeMoveInfoOracle(expected, slot, info);
            for (uint8_t byte = 0; byte < PackedMoveInfo::BYTE_COUNT; ++byte) {
                test.assert(actual[byte], expected[byte], "writer matches bitwise oracle");
            }
            test.assert(PackedMoveInfo::read(actual, slot), info,
                        "reader returns each ten-bit value");
            for (uint8_t other = 0; other < PackedMoveInfo::SLOT_COUNT; ++other) {
                test.assert(PackedMoveInfo::read(actual, other),
                            readMoveInfoOracle(expected, other),
                            "neighbor slots remain isolated");
            }
        }
    }

    PackedMoveInfo::write(actual, 0, 1023);
    PackedMoveInfo::write(actual, 0, 0);
    writeMoveInfoOracle(expected, 0, 1023);
    writeMoveInfoOracle(expected, 0, 0);
    test.assert(PackedMoveInfo::read(actual, 0), static_cast<uint16_t>(0),
                "overwrite maximum with zero clears slot");
    test.assert(PackedMoveInfo::read(nullptr, 0), static_cast<uint16_t>(0),
                "null packed record reads as zero");
    const uint8_t beforeInvalid[PackedMoveInfo::BYTE_COUNT] = {
        actual[0], actual[1], actual[2], actual[3], actual[4]};
    PackedMoveInfo::write(actual, PackedMoveInfo::SLOT_COUNT, 1023);
    test.assert(PackedMoveInfo::read(actual, PackedMoveInfo::SLOT_COUNT),
                static_cast<uint16_t>(0), "invalid slot reads as zero");
    for (uint8_t byte = 0; byte < PackedMoveInfo::BYTE_COUNT; ++byte) {
        test.assert(actual[byte], beforeInvalid[byte], "invalid write leaves record intact");
    }
    suite.addTest(test);
}

void MenuNavSuite(TestRunner &runner) {
    TestSuite suite("Menu navigation suite");
    MenuNavTest(suite);
    MenuIntentTest(suite);
    MenuSnapshotIntegrationTest(suite);
    PackedMoveInfoCodecTest(suite);
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


void MenuSnapshotIntegrationTest(TestSuite &suite) {
    Test test(__func__);
    MenuV2 menu;
    battle::BattleView view = {};
    const uint8_t player = static_cast<uint8_t>(battle::Side::Player);

    view.activeSlot[player] = 1;
    view.partyCount[player] = 3;
    view.party[player][0] = {0, 40, 1}; // species zero remains valid
    view.party[player][1] = {5, 60, 1};
    view.party[player][2] = {7, 22, 1};
    view.moveIds[0] = 0;
    view.moveIds[1] = 32;
    view.moveIds[2] = 3;
    view.moveIds[3] = 255;

    FxReadCounter::resetFrame();
    menu.openMenu(BATTLE_MOVE_SELECT, view);
    test.assert(menu.menuPointer, static_cast<int8_t>(0),
                "move snapshot opens one submenu");
    test.assert(menu.movesSnapshot().moveIds[0], static_cast<uint8_t>(0),
                "move zero copied at open");
    test.assert(menu.movesSnapshot().moveIds[1], static_cast<uint8_t>(32),
                "empty move sentinel copied at open");
    test.assert(menu.movesSnapshot().moveIds[2], static_cast<uint8_t>(3),
                "ordinary move copied at open");
    test.assert(menu.movesSnapshot().moveIds[3], static_cast<uint8_t>(255),
                "absent move sentinel copied at open");
    test.assert(menu.moveNameAddresses[0], static_cast<uint24_t>(0x20000),
                "move zero name resolved once");
    test.assert(menu.moveNameAddresses[1], static_cast<uint24_t>(0),
                "empty move has no name lookup");
    test.assert(menu.moveNameAddresses[2], static_cast<uint24_t>(0x20003),
                "ordinary move name resolved once");
    test.assert(menu.moveNameAddresses[3], static_cast<uint24_t>(0),
                "absent move has no name lookup");
    test.assert(FxReadCounter::count(), static_cast<uint8_t>(4),
                "open resolves each valid move name and record once");

    view.moveIds[0] = 9;
    view.party[player][0] = {11, 1, 1};
    FxReadCounter::resetFrame();
    for (uint8_t frame = 0; frame < 4; ++frame) {
        menu.update(0);
    }
    test.assert(FxReadCounter::count(), static_cast<uint8_t>(0),
                "steady menu updates perform no FX reads");
    test.assert(menu.movesSnapshot().moveIds[0], static_cast<uint8_t>(0),
                "post-open move source changes do not mutate snapshot");
    test.assert(menu.moveNameAddresses[0], static_cast<uint24_t>(0x20000),
                "post-open move source changes do not mutate addresses");

    view.moveIds[0] = 0;
    view.party[player][0] = {0, 40, 1};
    view.party[player][2] = {7, 22, 1};
    menu.clear();
    FxReadCounter::resetFrame();
    menu.openMenu(BATTLE_CREATURE_SELECT, view);
    test.assert(menu.partySnapshot().count, static_cast<uint8_t>(2),
                "party snapshot excludes active slot but keeps original rows");
    test.assert(menu.partySnapshot().choices[0].id, static_cast<uint8_t>(0),
                "species zero party ID copied");
    test.assert(menu.partySnapshot().choices[0].slot, static_cast<uint8_t>(0),
                "first party row keeps original slot");
    test.assert(menu.partySnapshot().choices[1].slot, static_cast<uint8_t>(2),
                "second party row keeps original slot");
    test.assert(menu.creatureNameAddresses[0], static_cast<uint24_t>(0x10000),
                "species zero name resolved at open");
    test.assert(menu.creatureNameAddresses[1], static_cast<uint24_t>(0x10007),
                "second party name resolved at open");
    test.assert(FxReadCounter::count(), static_cast<uint8_t>(2),
                "party names resolve once at open");

    view.party[player][0] = {12, 1, 1};
    view.party[player][2] = {13, 0, 0};
    FxReadCounter::resetFrame();
    menu.update(0);
    test.assert(FxReadCounter::count(), static_cast<uint8_t>(0),
                "party updates perform no FX reads");
    test.assert(menu.partySnapshot().choices[0].id, static_cast<uint8_t>(0),
                "post-open party source changes do not mutate ID");
    test.assert(menu.partySnapshot().choices[1].hp, static_cast<uint8_t>(22),
                "post-open party source changes do not mutate HP");
    menu.cursorIndex = 1;
    MenuIntent intent = menu.update(MENU_EDGE_A);
    test.assert(intent.kind, MenuIntentKind::SelectParty,
                "party snapshot emits SelectParty");
    test.assert(intent.index, static_cast<uint8_t>(2),
                "party intent preserves original slot");

    menu.clear();
    view.party[player][0] = {255, 30, 1};
    view.party[player][2] = {9, 20, 0};
    FxReadCounter::resetFrame();
    menu.openMenu(BATTLE_CREATURE_SELECT, view);
    test.assert(FxReadCounter::count(), static_cast<uint8_t>(0),
                "invalid/dead party rows do not resolve names");
    menu.cursorIndex = 0;
    intent = menu.update(MENU_EDGE_A);
    test.assert(intent.kind, MenuIntentKind::None,
                "invalid party ID cannot emit intent");
    menu.cursorIndex = 1;
    intent = menu.update(MENU_EDGE_A);
    test.assert(intent.kind, MenuIntentKind::None,
                "dead party row cannot emit intent");
    test.assert(menu.menuPointer, static_cast<int8_t>(0),
                "invalid/dead choices retain menu ownership");

    suite.addTest(test);
}
