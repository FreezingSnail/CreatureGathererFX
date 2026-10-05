#pragma once

#include "test.hpp"
#include "../src/engine/battle/BattleView.hpp"
#include "../src/engine/menu/MenuIntent.hpp"

#include <type_traits>

static_assert(sizeof(battle::Side) == 1, "battle side must fit in one byte");
static_assert(sizeof(battle::ActiveView) == 3, "active view size is part of the contract");
static_assert(sizeof(battle::PartySummary) == 3, "party summary size is part of the contract");
static_assert(sizeof(battle::BattleView) == 39, "battle view size is part of the contract");
static_assert(sizeof(battle::MoveSnapshot) == 9, "move snapshot size is part of the contract");
static_assert(sizeof(battle::PartyChoice) == 3, "party choice size is part of the contract");
static_assert(sizeof(battle::PartySnapshot) == 8, "party snapshot size is part of the contract");
static_assert(sizeof(MenuIntentKind) == 1, "menu intent kind must fit in one byte");
static_assert(sizeof(MenuIntent) == 2, "menu intent size is part of the contract");
static_assert(std::is_trivially_copyable<battle::BattleView>::value,
              "battle view must remain a plain copied value");

void BattleViewContractTest(TestSuite &suite) {
    Test test(__func__);

    battle::BattleView view = {};
    view.active[static_cast<uint8_t>(battle::Side::Player)] = {7, 23, 31};
    view.active[static_cast<uint8_t>(battle::Side::Opponent)] = {12, 0, 18};
    view.party[static_cast<uint8_t>(battle::Side::Player)][1] = {9, 4, 1};
    view.partyCount[static_cast<uint8_t>(battle::Side::Player)] = 2;
    view.activeSlot[static_cast<uint8_t>(battle::Side::Player)] = 0;
    view.moveIds[0] = 0;
    view.moveIds[1] = 32;
    view.moveIds[2] = 255;
    view.gatherProgress = 3;
    view.gatherNeed = 5;

    battle::MoveSnapshot moves = {{view.moveIds[0], view.moveIds[1], 4, 255}};
    battle::PartySnapshot party = {{{9, 1, 4}, {11, 2, 8}}, 2, 1};
    MenuIntent intent = {MenuIntentKind::SelectParty, party.choices[0].slot};

    test.assert(view.active[0].id, static_cast<uint8_t>(7), "player active ID copied by value");
    test.assert(view.active[1].hp, static_cast<uint8_t>(0), "fainted active HP represented");
    test.assert(view.party[0][1].alive, static_cast<uint8_t>(1), "party alive flag represented");
    test.assert(view.partyCount[0], static_cast<uint8_t>(2), "party count represented");
    test.assert(moves.moveIds[0], static_cast<uint8_t>(0), "move ID zero remains valid");
    test.assert(moves.moveIds[1], static_cast<uint8_t>(32), "empty move table entry is preserved");
    test.assert(moves.moveIds[3], static_cast<uint8_t>(255), "absent move sentinel is preserved");
    test.assert(party.choices[0].slot, static_cast<uint8_t>(1), "party choice keeps original slot");
    test.assert(party.forced, static_cast<uint8_t>(1), "forced replacement flag represented");
    test.assert(intent.kind, MenuIntentKind::SelectParty, "menu emits a typed intent");
    test.assert(intent.index, static_cast<uint8_t>(1), "menu intent keeps selected slot");
    test.assert(view.gatherProgress, static_cast<uint8_t>(3), "gather progress represented");
    test.assert(view.gatherNeed, static_cast<uint8_t>(5), "gather target represented");

    suite.addTest(test);
}

void BattleViewSuite(TestRunner &runner) {
    TestSuite suite("Battle view and menu intent contract suite");
    BattleViewContractTest(suite);
    runner.addTestSuite(suite);
}
