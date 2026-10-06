#pragma once

#include "test.hpp"
#include "src/engine/arena/ArenaNavigation.hpp"
#include "src/engine/arena/ArenaCatalog.hpp"
#include "src/engine/menu/MenuV2.hpp"
#include "fxdata/generated/arena_demo_ids.hpp"

inline void ArenaFlowSuite(TestRunner &runner)
{
    Test test("Arena selection and replay flow");
    arena::ArenaUiState ui{};
    arena::ArenaContext context{};
    ui.screen = arena::ArenaScreen::PlayerTeam;
    ui.list = {ArenaDemoIds::playerCount, 1, 0, 0};

    test.assert(arena::navigate(ui, context, MENU_EDGE_A,
                                ArenaDemoIds::playerCount,
                                ArenaDemoIds::opponentCount),
                arena::ArenaIntent::PreviewChanged,
                "first confirm opens opponent preview only");
    test.assert(ui.screen, arena::ArenaScreen::OpponentTeam,
                "team confirmation opens the opponent list");
    test.assert(context.playerTeam, static_cast<uint8_t>(0),
                "opening opponents commits the selected player team");
    test.assert(arena::navigate(ui, context, MENU_EDGE_A,
                                ArenaDemoIds::playerCount,
                                ArenaDemoIds::opponentCount),
                arena::ArenaIntent::StartBattle,
                "second fresh confirm starts the selected matchup");
    test.assert(ui.screen, arena::ArenaScreen::OpponentTeam,
                "battle intent preserves opponent screen until caller starts");
    test.assert(context.opponentTeam, static_cast<uint8_t>(0),
                "opponent confirmation preserves the selection");

    for (uint8_t playerTeam = 0; playerTeam < ArenaDemoIds::playerCount;
         ++playerTeam) {
        for (uint8_t opponentTeam = 0;
             opponentTeam < ArenaDemoIds::opponentCount; ++opponentTeam) {
            context.playerTeam = playerTeam;
            context.opponentTeam = opponentTeam;
            ui.screen = arena::ArenaScreen::PlayerTeam;
            ui.list = {ArenaDemoIds::playerCount, 1, playerTeam, playerTeam};
            const arena::ArenaIntent openOpponent = arena::navigate(
                ui, context, MENU_EDGE_A, ArenaDemoIds::playerCount,
                ArenaDemoIds::opponentCount);
            test.assert(openOpponent, arena::ArenaIntent::PreviewChanged,
                        "each matchup requires team confirmation before opponent selection");
            ui.list.cursor = opponentTeam;
            const arena::ArenaIntent start = arena::navigate(
                ui, context, MENU_EDGE_A, ArenaDemoIds::playerCount,
                ArenaDemoIds::opponentCount);
            test.assert(start, arena::ArenaIntent::StartBattle,
                        "all fifteen selected pairs produce a battle intent");
            test.assert(context.playerTeam, playerTeam,
                        "player selection survives matchup navigation");
            test.assert(context.opponentTeam, opponentTeam,
                        "opponent selection survives matchup navigation");
        }
    }

    TestSuite suite("Arena flow suite");
    suite.addTest(test);
    runner.addTestSuite(suite);
}
