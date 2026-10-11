#pragma once

#include "test.hpp"
#include "../src/engine/battle/BattleFlow.hpp"
#include "../src/engine/battle/MoveUses.hpp"
#include "../src/engine/ModeState.hpp"
#include "../src/engine/menu/MenuNav.hpp"
#include "../src/globals.hpp"

namespace battle_flow_test_detail {

uint8_t terminalCallbacks = 0;
battle::Outcome callbackOutcome = battle::Outcome::None;
uint16_t rngCalls = 0;
void onTerminal(battle::Outcome outcome) {
    ++terminalCallbacks;
    callbackOutcome = outcome;
}

uint8_t flowRng(uint8_t) { ++rngCalls; return 0; }

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
    battleSession().setRng({flowRng});
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
    battleSession().setRng({flowRng});
    battleSession().beginWild(4, 1, true, 1);
}

void finishActionTurn() {
    for (uint16_t frame = 0; frame < 4096 &&
         !battleSession().awaitingPlayer() && !battleSession().exitReady(); ++frame) {
        BattleFlow::update(0);
    }
}

struct FeedbackRun {
    uint16_t actionRngCalls;
    uint16_t rngCalls;
    battle::ResultKind resultKind;
    battle::Outcome outcome;
    uint8_t playerHp;
    uint8_t opponentHp;
    uint8_t moveUses;
};

FeedbackRun runFeedback(bool accelerate) {
    beginTrainer();
    BattleFlow::update(0); // Open the move page.
    BattleFlow::update(MENU_EDGE_A); // Submit the default usable move.
    BattleFlow::update(0); // Resolve once and begin feedback.

    FeedbackRun run = {};
    run.actionRngCalls = rngCalls;
    run.resultKind = battleSession().result().kind;
    run.outcome = battleSession().result().outcome;
    run.playerHp = battleSession().state().active[0].hp;
    run.opponentHp = battleSession().state().active[1].hp;
    run.moveUses = battle::remainingMoveUses(
        battleSession().state(), battle::Side::Player, 0);

    if (accelerate) {
        for (uint8_t tick = 0; tick < 4; ++tick) BattleFlow::update(0);
        BattleFlow::update(MENU_EDGE_A);
    } else {
        for (uint8_t tick = 0; tick < 26; ++tick) BattleFlow::update(0);
    }
    run.rngCalls = rngCalls;
    run.playerHp = battleSession().state().active[0].hp;
    run.opponentHp = battleSession().state().active[1].hp;
    run.moveUses = battle::remainingMoveUses(
        battleSession().state(), battle::Side::Player, 0);
    exitBattle();
    gameState.state = GameState_t::WORLD;
    return run;
}

} // namespace battle_flow_test_detail

inline void BattleFlowControllerTest(TestSuite &suite) {
    using namespace battle_flow_test_detail;
    Test test(__func__);

    const FeedbackRun natural = runFeedback(false);
    const FeedbackRun accelerated = runFeedback(true);
    test.assert(accelerated.rngCalls, natural.rngCalls,
                "natural and accelerated feedback consume identical RNG");
    test.assert(natural.rngCalls, natural.actionRngCalls,
                "natural feedback ticks do not rerun the action");
    test.assert(accelerated.rngCalls, accelerated.actionRngCalls,
                "accelerated feedback ticks do not rerun the action");
    test.assert(accelerated.resultKind, natural.resultKind,
                "feedback timing preserves the action result kind");
    test.assert(accelerated.outcome, natural.outcome,
                "feedback timing preserves the action outcome");
    test.assert(accelerated.playerHp, natural.playerHp,
                "feedback timing preserves player state");
    test.assert(accelerated.opponentHp, natural.opponentHp,
                "feedback timing preserves opponent state");
    test.assert(accelerated.moveUses, natural.moveUses,
                "feedback timing preserves move uses");

    beginTrainer();
    test.assert(BattleFlow::update(0), false, "trainer enters compact move page");
    test.assert(modeState.battle.ui.menu.initialized(), true,
                "menu state is initialized on the choice boundary");
    test.assert(BattleFlow::update(MENU_EDGE_A), false,
                "fresh A submits one move without resolving in that frame");
    test.assert(battleSession().result().kind, battle::ResultKind::None,
                "selection does not combine with the action boundary");
    test.assert(BattleFlow::update(MENU_EDGE_A), false,
                "the next frame resolves once and consumes its A edge");
    test.assert(battleSession().isPresenting(), true,
                "a resolved result owns the feedback union arm");
    const uint8_t hpAfterAction = battleSession().state().active[0].hp;
    BattleFlow::update(MENU_EDGE_A);
    test.assert(battleSession().isPresenting(), true,
                "an early feedback A edge cannot skip minimum dwell");
    test.assert(battleSession().state().active[0].hp, hpAfterAction,
                "feedback input does not rerun the action");
    finishActionTurn();
    test.assert(battleSession().awaitingPlayer(), true,
                "feedback completion returns to a choice boundary");
    test.assert(modeState.battle.ui.menu.initialized(), false,
                "feedback completion constructs a fresh uninitialized menu");
    BattleFlow::update(0);
    test.assert(modeState.battle.ui.menu.initialized(), true,
                "next frame prepares the next choice menu");
    exitBattle();
    gameState.state = GameState_t::WORLD;

    beginWild();
    terminalCallbacks = 0;
    callbackOutcome = battle::Outcome::None;
    BattleFlow::update(0); // Prepare moves; the entry edge is discarded.
    BattleFlow::update(MENU_EDGE_B); // Open actions, default Gather selection.
    BattleFlow::update(MENU_EDGE_A); // Submit Gather.
    BattleFlow::update(0); // Resolve one action.
    test.assert(battleSession().result().kind, battle::ResultKind::Gather,
                "wild action page routes Gather through the session");
    test.assert(battleSession().isPresenting(), true,
                "gather result uses the same feedback phase");
    finishActionTurn();
    BattleFlow::update(0); // Prepare the next move page.
    BattleFlow::update(MENU_EDGE_B);
    BattleFlow::update(MENU_NAV_RIGHT);
    BattleFlow::update(MENU_EDGE_A);
    BattleFlow::update(0);
    test.assert(battleSession().result().kind, battle::ResultKind::Escape,
                "wild action page routes Escape through the session");
    test.assert(battleSession().result().outcome, battle::Outcome::Escaped,
                "successful escape is terminal");
    test.assert(BattleFlow::update(0, onTerminal), false,
                "terminal feedback begins before the callback");
    test.assert(terminalCallbacks, static_cast<uint8_t>(0),
                "terminal callback waits for feedback completion");
    for (uint8_t tick = 0; tick < 26; ++tick)
        BattleFlow::update(0, onTerminal);
    test.assert(terminalCallbacks, static_cast<uint8_t>(1),
                "terminal callback runs exactly once");
    test.assert(callbackOutcome, battle::Outcome::Escaped,
                "terminal callback receives the resolved outcome");
    test.assert(gameState.state, GameState_t::BATTLE,
                "callback ownership leaves the mode transition to caller");
    exitBattle();
    gameState.state = GameState_t::WORLD;

    suite.addTest(test);
}

inline void BattleFlowSuite(TestRunner &runner) {
    TestSuite suite("Slim battle frame controller");
    BattleFlowControllerTest(suite);
    runner.addTestSuite(suite);
}
