#pragma once

#include "test.hpp"
#include "../src/engine/arena/ArenaDemo.hpp"
#include "../src/engine/ModeState.hpp"
#include "../src/engine/battle/BattleFlow.hpp"
#include "../src/engine/menu/MenuV2.hpp"
#include "../src/globals.hpp"

namespace arena_lifecycle_test_detail {

void routeFinishedBattle(battle::Outcome outcome)
{
    arena::finishBattle(outcome);
}

} // namespace arena_lifecycle_test_detail

inline void ArenaLifecycleSuite(TestRunner &runner)
{
    using namespace arena_lifecycle_test_detail;
    TestSuite suite("Arena mode lifecycle");
    Test test("arena terminal return and persistent selection");

    arena::boot();
    test.assert(gameState.state, GameState_t::ARENA,
                "arena bootstrap enters arena mode");
    test.assert(static_cast<uint8_t>(modeState.arena.ui.screen),
                static_cast<uint8_t>(arena::ArenaScreen::PlayerTeam),
                "arena bootstrap opens player selection");
    arena::arenaContext.playerTeam = 1;
    arena::arenaContext.opponentTeam = 1;

    player.basic();
    menu.clear();
    dialogMenu.clear();
    enterBattle();
    gameState.state = GameState_t::BATTLE;
    battleSession().beginWild(4, 1, true, 1);
    test.assert(battleSession().submitIntent({MenuIntentKind::Escape, 0}), true,
                "fixture submits a real terminal battle outcome");

    BattleFlow::update(0, routeFinishedBattle);
    uint16_t frames = 0;
    bool routed = false;
    while (frames < 4096 && !routed) {
        routed = BattleFlow::update(0, routeFinishedBattle);
        ++frames;
    }
    test.assert(routed, true, "terminal presentation invokes arena destination");
    test.assert(gameState.state, GameState_t::ARENA,
                "arena terminal callback does not initialize overworld");
    test.assert(arena::arenaContext.playerTeam, static_cast<uint8_t>(1),
                "player selection survives BattleMode destruction");
    test.assert(arena::arenaContext.opponentTeam, static_cast<uint8_t>(1),
                "opponent selection survives BattleMode destruction");
    test.assert(static_cast<uint8_t>(arena::arenaContext.outcome),
                static_cast<uint8_t>(battle::Outcome::Escaped),
                "callback captures outcome before BattleMode destruction");
    test.assert(static_cast<uint8_t>(modeState.arena.ui.screen),
                static_cast<uint8_t>(arena::ArenaScreen::Result),
                "callback opens result view after playback");
    test.assert(frames < 4096, true, "terminal playback remains bounded");

    suite.addTest(test);
    runner.addTestSuite(suite);
}
