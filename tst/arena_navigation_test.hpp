#pragma once

#include "test.hpp"
#include "../src/engine/arena/ArenaNavigation.hpp"
#include "../src/engine/menu/MenuV2.hpp"
#include <cstring>

inline void ArenaNavigationSuite(TestRunner &runner)
{
    TestSuite suite("Arena navigation");
    Test test("arena screen transitions, bounds, and selection ownership");
    using namespace arena;

    ArenaContext context = {1, 3, battle::Outcome::Win};
    ArenaUiState ui = {};
    ui.screen = ArenaScreen::PlayerTeam;
    ui.list = {3, 1, 0, 0};
    test.assert(navigate(ui, context, MENU_EDGE_A, 3, 5),
                ArenaIntent::PreviewChanged, "first A advances to opponent selection");
    test.assert(context.playerTeam, static_cast<uint8_t>(0),
                "first team index zero is committed");
    test.assert(ui.screen, ArenaScreen::OpponentTeam,
                "first A opens opponent screen");
    test.assert(ui.list.cursor, static_cast<uint8_t>(3),
                "saved opponent selection is restored");
    test.assert(context.outcome, battle::Outcome::Win,
                "selection transition preserves outcome");

    test.assert(navigate(ui, context, MENU_EDGE_A, 3, 5), ArenaIntent::StartBattle,
                "second A starts battle");
    test.assert(context.opponentTeam, static_cast<uint8_t>(3),
                "battle confirmation commits opponent selection");

    ui.screen = ArenaScreen::PlayerTeam;
    ui.list = {3, 1, 0, 1};
    context.playerTeam = 2;
    test.assert(navigate(ui, context, MENU_EDGE_A | MENU_EDGE_B, 3, 5),
                ArenaIntent::None, "B takes priority over A on player screen");
    test.assert(ui.screen, ArenaScreen::PlayerTeam,
                "player-screen B keeps the current screen");
    test.assert(navigate(ui, context, MENU_NAV_UP | MENU_NAV_DOWN, 3, 5),
                ArenaIntent::PreviewChanged, "Up takes priority over Down");
    test.assert(ui.list.cursor, static_cast<uint8_t>(0), "Up moves to first team");
    test.assert(navigate(ui, context, MENU_NAV_UP, 3, 5), ArenaIntent::None,
                "movement at first entry is clamped");
    test.assert(navigate(ui, context, MENU_NAV_LEFT | MENU_NAV_RIGHT, 3, 5),
                ArenaIntent::None, "left and right are ignored");
    ArenaUiState before = ui;
    test.assert(navigate(ui, context, 0, 3, 5), ArenaIntent::None,
                "zero edge mask is ignored");
    test.assert(std::memcmp(&ui, &before, sizeof(ui)) == 0, true,
                "zero edge mask leaves UI unchanged");

    // Player choice is independent of current preview movement. A confirms
    // the highlighted index, then opponent B returns to the saved player team.
    ui.list = {3, 1, 0, 2};
    test.assert(navigate(ui, context, MENU_EDGE_A, 3, 5),
                ArenaIntent::PreviewChanged, "player confirmation opens opponents");
    test.assert(context.playerTeam, static_cast<uint8_t>(2),
                "player confirmation saves highlighted team");
    test.assert(navigate(ui, context, MENU_EDGE_B, 3, 5),
                ArenaIntent::PreviewChanged, "opponent B backs to players");
    test.assert(ui.screen, ArenaScreen::PlayerTeam, "B restores player selection");
    test.assert(ui.list.cursor, static_cast<uint8_t>(2), "B restores saved team index");

    ui.screen = ArenaScreen::OpponentTeam;
    ui.list = {5, 1, 4, 4};
    test.assert(navigate(ui, context, MENU_NAV_DOWN, 3, 5), ArenaIntent::None,
                "movement at last opponent is clamped");
    test.assert(ui.list.cursor, static_cast<uint8_t>(4), "last opponent index is valid");
    test.assert(navigate(ui, context, MENU_EDGE_A, 3, 5), ArenaIntent::StartBattle,
                "last opponent can be confirmed");
    test.assert(context.opponentTeam, static_cast<uint8_t>(4),
                "last opponent selection is committed");

    ui.screen = ArenaScreen::Result;
    ui.list = {3, 3, 0, 0};
    test.assert(navigate(ui, context, MENU_NAV_DOWN, 3, 5), ArenaIntent::None,
                "result cursor moves without preview intent");
    test.assert(ui.list.cursor, static_cast<uint8_t>(1), "result reaches change opponent");
    test.assert(navigate(ui, context, MENU_EDGE_A, 3, 5),
                ArenaIntent::PreviewChanged, "change opponent opens opponent screen");
    test.assert(ui.list.cursor, static_cast<uint8_t>(4),
                "change opponent restores saved opponent");
    test.assert(navigate(ui, context, MENU_EDGE_B, 3, 5),
                ArenaIntent::PreviewChanged, "opponent B returns to saved team");
    test.assert(ui.list.cursor, static_cast<uint8_t>(2), "team index remains saved");
    ui.screen = ArenaScreen::Result;
    ui.list = {3, 3, 0, 0};
    test.assert(navigate(ui, context, MENU_EDGE_B, 3, 5),
                ArenaIntent::PreviewChanged, "result B opens opponent selection");

    ui.screen = ArenaScreen::Result;
    ui.list = {3, 3, 0, 0};
    test.assert(navigate(ui, context, MENU_EDGE_A, 3, 5), ArenaIntent::StartBattle,
                "rematch starts using saved context");
    test.assert(context.playerTeam, static_cast<uint8_t>(2),
                "rematch preserves player team");
    test.assert(context.opponentTeam, static_cast<uint8_t>(4),
                "rematch preserves opponent team");
    test.assert(context.outcome, battle::Outcome::Win, "rematch preserves outcome");
    ui.list.cursor = 2;
    test.assert(navigate(ui, context, MENU_EDGE_A, 3, 5),
                ArenaIntent::PreviewChanged, "change team opens player screen");
    test.assert(ui.list.cursor, static_cast<uint8_t>(2), "change team restores saved team");

    ui.screen = ArenaScreen::PlayerTeam;
    ui.list = {255, 1, 200, 254};
    test.assert(navigate(ui, context, MENU_NAV_DOWN, 255, 255),
                ArenaIntent::None, "255-count end is clamped");
    test.assert(ui.list.itemCount, static_cast<uint8_t>(255), "byte maximum count retained");
    test.assert(ui.list.cursor, static_cast<uint8_t>(254), "byte maximum index remains valid");

    ui.screen = ArenaScreen::PlayerTeam;
    ui.list = {3, 1, 0, 2};
    test.assert(navigate(ui, context, MENU_EDGE_A, 0, 0), ArenaIntent::None,
                "empty catalog cannot enter a match");
    test.assert(ui.list.itemCount, static_cast<uint8_t>(0),
                "empty player catalog clears visible list count");
    test.assert(ui.list.cursor, static_cast<uint8_t>(0),
                "empty player catalog clears cursor");
    ui.screen = ArenaScreen::OpponentTeam;
    ui.list = {1, 1, 0, 0};
    test.assert(navigate(ui, context, MENU_EDGE_A, 3, 0), ArenaIntent::None,
                "empty opponent catalog cannot start battle");
    test.assert(ui.list.itemCount, static_cast<uint8_t>(0),
                "empty opponent catalog clears list count");
    test.assert(ui.list.cursor, static_cast<uint8_t>(0),
                "empty opponent catalog clears cursor");
    ui.screen = ArenaScreen::OpponentTeam;
    ui.list = {0, 1, 0, 0};
    test.assert(navigate(ui, context, MENU_EDGE_B, 0, 5), ArenaIntent::None,
                "empty player catalog returns without requesting a preview read");
    test.assert(ui.screen, ArenaScreen::PlayerTeam,
                "back navigation still reaches the empty player screen");
    test.assert(ui.list.itemCount, static_cast<uint8_t>(0),
                "empty player screen keeps its count cleared");

    suite.addTest(test);
    runner.addTestSuite(suite);
}
