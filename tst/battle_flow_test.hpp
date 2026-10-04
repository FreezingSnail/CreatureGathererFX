#pragma once

#include "test.hpp"
#include "../src/engine/battle/BattleFlow.hpp"
#include "../src/engine/ModeState.hpp"
#include "../src/engine/menu/MenuNav.hpp"
#include "../src/engine/menu/MenuV2.hpp"
#include "../src/globals.hpp"

namespace battle_flow_test_detail {

uint8_t rngCalls = 0;
uint8_t rngZero(uint8_t) { ++rngCalls; return 0; }

void beginTrainer() {
    player = Player();
    player.loadCreature(0, 1);
    player.loadCreature(1, 2);
    player.loadCreature(2, 3);
    menu.clear();
    dialogMenu.clear();
    enterBattle();
    gameState.state = GameState_t::BATTLE;
    rngCalls = 0;
    battleSession().setRng({rngZero});
    battleSession().beginTrainer(0);
}

void beginWild() {
    player = Player();
    player.loadCreature(0, 1);
    player.loadCreature(1, 2);
    player.creatureHPs[0] = 27;
    menu.clear();
    dialogMenu.clear();
    enterBattle();
    gameState.state = GameState_t::BATTLE;
    rngCalls = 0;
    battleSession().setRng({rngZero});
    battleSession().beginWild(4, 1, true, 1);
}

} // namespace battle_flow_test_detail

inline void BattleFlowControllerTest(TestSuite &suite) {
    using namespace battle_flow_test_detail;
    Test test(__func__);

    beginTrainer();
    test.assert(BattleFlow::update(0), false, "choice frame stays in battle");
    test.assert(menu.stack[menu.menuPointer], BATTLE_OPTIONS,
                "shared controller opens battle options");
    test.assert(BattleFlow::update(MENU_EDGE_A), false,
                "first A opens move submenu without resolving");
    test.assert(menu.stack[menu.menuPointer], BATTLE_MOVE_SELECT,
                "move submenu is active after first A");
    test.assert(battlePresenter().stage(), battle::PresenterStage::Idle,
                "submenu opening edge does not begin playback");
    test.assert(BattleFlow::update(MENU_EDGE_A), false,
                "second fresh A chooses a move");
    test.assert(battlePresenter().stage(), battle::PresenterStage::Announce,
                "move selection begins the real presenter");
    const uint8_t resolvedCalls = rngCalls;
    test.assert(battleSession().result().kind, battle::ResultKind::Attack,
                "move resolves one attack result");
    test.assert(BattleFlow::update(MENU_EDGE_A), false,
                "presenter owns the next A edge");
    test.assert(menu.menuPointer, static_cast<int8_t>(-1),
                "playback A cannot open a menu");
    test.assert(rngCalls, resolvedCalls,
                "playback frames never reroll the action");

    uint16_t frames = 0;
    while (frames < 4096 && gameState.state == GameState_t::BATTLE &&
           !battleSession().awaitingPlayer() && !battleSession().exitReady()) {
        BattleFlow::update(0);
        ++frames;
    }
    test.assert(frames < 4096, true, "trainer action playback is bounded");
    test.assert(battleSession().awaitingPlayer() || battleSession().exitReady(),
                true, "shared flow returns to choice only after playback");
    test.assert(menu.menuPointer, static_cast<int8_t>(-1),
                "completion edge does not select a new action");
    exitBattle();
    gameState.state = GameState_t::WORLD;

    beginWild();
    test.assert(battleSession().state().active[0].hp,
                static_cast<uint8_t>(27), "entry imports damaged persistent HP");
    test.assert(battleSession().submitIntent({MenuIntentKind::Escape, 0}), true,
                "wild escape submits through session");
    BattleFlow::update(0);
    test.assert(gameState.state, GameState_t::BATTLE,
                "terminal result remains resident during feedback");
    frames = 0;
    bool exited = false;
    while (frames < 4096 && !exited) {
        exited = BattleFlow::update(0);
        ++frames;
    }
    test.assert(exited, true, "controller exits after terminal feedback");
    test.assert(gameState.state, GameState_t::WORLD,
                "terminal exit restores world state");
    test.assert(player.creatureHPs[0], static_cast<uint8_t>(27),
                "terminal exit keeps persistent HP");
    test.assert(menu.menuPointer, static_cast<int8_t>(-1),
                "terminal exit clears battle menu");

    suite.addTest(test);
}

inline void BattleFlowSuite(TestRunner &runner) {
    TestSuite suite("Battle frame controller");
    BattleFlowControllerTest(suite);
    runner.addTestSuite(suite);
}
