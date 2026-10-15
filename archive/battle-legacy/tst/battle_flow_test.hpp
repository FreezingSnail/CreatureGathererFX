#pragma once

#include "test.hpp"
#include "../src/engine/battle/BattleFlow.hpp"
#include "../src/engine/battle/MoveUses.hpp"
#include "../src/engine/ModeState.hpp"
#include "../src/engine/menu/MenuNav.hpp"
#include "../src/engine/world/World.hpp"
#include "../src/WorldInteractionFake.hpp"
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
    BattleFlow::update(0); // Prepare the options page.
    BattleFlow::update(MENU_EDGE_A); // Open the four-slot move page.
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
        for (uint8_t tick = 0; tick < battle::FEEDBACK_MIN_DWELL_TICKS; ++tick)
            BattleFlow::update(0);
        BattleFlow::update(MENU_EDGE_A);
    } else {
        for (uint8_t tick = 0; tick < battle::FEEDBACK_HOLD_TICKS; ++tick)
            BattleFlow::update(0);
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
    test.assert(BattleFlow::update(0), false,
                "trainer enters the compact options page");
    test.assert(modeState.battle.ui.menu.initialized(), true,
                "menu state is initialized on the choice boundary");
    test.assert(BattleFlow::update(MENU_EDGE_A), false,
                "A on MOVE opens the four-slot move page");
    test.assert(battleSession().result().kind, battle::ResultKind::None,
                "opening the move page does not resolve");
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

    // The end-turn step is acknowledged without a feedback caption or dwell.
    BattleFlow::update(MENU_EDGE_A); // Open the move page.
    BattleFlow::update(MENU_EDGE_A); // Submit the next action.
    bool endTurnPresented = false;
    for (uint16_t frame = 0; frame < 4096 && !battleSession().awaitingPlayer(); ++frame) {
        BattleFlow::update(MENU_EDGE_A);
        if (battleSession().isPresenting() &&
            battleSession().result().kind == battle::ResultKind::EndTurn) {
            endTurnPresented = true;
        }
    }
    test.assert(endTurnPresented, false,
                "end-turn results never enter the feedback phase");
    test.assert(battleSession().awaitingPlayer(), true,
                "the turn reaches the next choice boundary");
    exitBattle();
    gameState.state = GameState_t::WORLD;

    // CHANGE routes a voluntary switch through the session.
    beginTrainer();
    BattleFlow::update(0); // Prepare the options page.
    BattleFlow::update(MENU_NAV_DOWN);
    BattleFlow::update(MENU_EDGE_A);
    test.assert(modeState.battle.ui.menu.partyPending(), false,
                "flow caches the party rows on the CHANGE transition");
    test.assert(BattleFlow::update(MENU_EDGE_A), false,
                "party row submits a voluntary switch");
    test.assert(battleSession().isPresenting(), false,
                "switch selection does not resolve in the same frame");
    BattleFlow::update(0);
    test.assert(battleSession().result().kind, battle::ResultKind::Switch,
                "voluntary switch resolves as its own result");
    test.assert(battleSession().state().activeSlot[0] != 0, true,
                "voluntary switch changes the active slot");
    exitBattle();
    gameState.state = GameState_t::WORLD;

    beginWild();
    terminalCallbacks = 0;
    callbackOutcome = battle::Outcome::None;
    BattleFlow::update(0); // Prepare options; the entry edge is discarded.
    BattleFlow::update(MENU_NAV_RIGHT); // Gather cell.
    BattleFlow::update(MENU_EDGE_A); // Submit Gather.
    BattleFlow::update(0); // Resolve one action.
    test.assert(battleSession().result().kind, battle::ResultKind::Gather,
                "wild options page routes Gather through the session");
    test.assert(battleSession().isPresenting(), true,
                "gather result uses the same feedback phase");
    finishActionTurn();
    BattleFlow::update(0); // Prepare the next options page.
    BattleFlow::update(MENU_NAV_DOWN);
    BattleFlow::update(MENU_NAV_RIGHT);
    BattleFlow::update(MENU_EDGE_A);
    BattleFlow::update(0);
    test.assert(battleSession().result().kind, battle::ResultKind::Escape,
                "wild options page routes Escape through the session");
    test.assert(battleSession().result().outcome, battle::Outcome::Escaped,
                "successful escape is terminal");
    test.assert(BattleFlow::update(0, onTerminal), false,
                "terminal feedback begins before the callback");
    test.assert(terminalCallbacks, static_cast<uint8_t>(0),
                "terminal callback waits for feedback completion");
    for (uint8_t tick = 0; tick < battle::FEEDBACK_HOLD_TICKS; ++tick)
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

// Drive one real terminal Lose through BattleFlow with a forced single-member
// party wipe, then prove the deferred world recovery. `trainer` selects the
// fixed-trainer path so wild and boss defeat share the same proof.
inline void driveDefeatRecovery(TestSuite &suite, bool trainer, const char *label) {
    using namespace battle;
    using namespace battle_flow_test_detail;
    Test test(label);

    player = Player();
    player.loadCreature(0, 1);
    player.loadCreature(1, 2);
    player.loadCreature(2, 3);
    // Pre-battle tile faces the authored rest point (10,4) so a reused edge
    // would be observable if recovery did not relocate first.
    gameState.playerLocation = static_cast<uint16_t>((5U << 8) | 10U);
    menu.clear();
    dialogMenu.clear();
    enterBattle();
    gameState.state = GameState_t::BATTLE;
    battleSession().setRng({flowRng});
    if (trainer) battleSession().beginTrainer(0);
    else battleSession().beginWild(4, 1, false, 0);

    battle::BattleState &state = battleSession().stateForTest();
    Combatant &p = state.active[static_cast<uint8_t>(Side::Player)];
    Combatant &o = state.active[static_cast<uint8_t>(Side::Opponent)];
    p.hp = 1;
    p.stats.speed = 1;
    p.moveIds[0] = 0;
    p.moves[0] = Move(MoveBitSet{static_cast<uint8_t>(Type::SPIRIT), 1, 1, 0, 0});
    o.stats.speed = 255;
    o.moveIds[0] = 0;
    o.moves[0] = Move(MoveBitSet{static_cast<uint8_t>(Type::SPIRIT), 255, 1, 0, 0});
    state.bench[static_cast<uint8_t>(Side::Player)][0].hp = 0;
    state.bench[static_cast<uint8_t>(Side::Player)][1].hp = 0;

    BattleFlow::update(0);            // Prepare the options page.
    BattleFlow::update(MENU_EDGE_A);  // Open the move page.
    BattleFlow::update(MENU_EDGE_A);  // Submit the default usable move.

    // The shipping loop polls justPressed each frame and hands the battle its
    // fresh A edge, so a terminal dismiss completes on an A-edge frame. Model
    // the same physical press here: A is held for every battle frame, and the
    // world fake is left asserting until the completion frame is inspected.
    worldInteractionFake::pressedA = true;
    bool returned = false;
    for (uint16_t frame = 0; frame < 8192 && !returned; ++frame)
        returned = BattleFlow::update(MENU_EDGE_A);
    test.assert(returned, true, "terminal defeat returns to the world");
    test.assert(gameState.state, GameState_t::WORLD, "defeat restores the world mode");
    // run() returns immediately after the completion return, so the same A edge
    // never reaches the world that frame: recovery stays pending, no script slot
    // is read, and the party is still knocked out.
    test.assert(worldInteractionFake::readCount, static_cast<uint16_t>(0),
                "completion-frame A edge never reaches the world");
    test.assert(gameState.playerLocation, static_cast<uint16_t>((5U << 8) | 10U),
                "defeat defers relocation past the terminal frame");
    for (uint8_t i = 0; i < 3; ++i)
        test.assert(player.creatureHPs[i], static_cast<uint8_t>(0),
                    "defeat leaves the party knocked out until recovery");

    // Next polled frame: the physical A is still held, so the world's fresh
    // justPressed is false. Reset models that fresh poll before world dispatch.
    worldInteractionFake::reset();
    WorldEngine::runMap(worldState());
    test.assert(WorldEngine::location(), static_cast<uint16_t>((5U << 8) | 11U),
                "recovery returns to the authored first-beta start");
    for (uint8_t i = 0; i < 3; ++i)
        test.assert(player.creatureHPs[i], player.party[i].statlist.hp,
                    "recovery fully restores every occupied member");
    test.assert(worldInteractionFake::readCount, static_cast<uint16_t>(0),
                "recovery frame runs no interaction or script read");

    // A second frame is a no-op: recovery is not re-armed and does not step.
    WorldEngine::runMap(worldState());
    test.assert(WorldEngine::location(), static_cast<uint16_t>((5U << 8) | 11U),
                "recovery does not retrigger movement");
    test.assert(worldInteractionFake::readCount, static_cast<uint16_t>(0),
                "recovery does not retrigger interaction");
    suite.addTest(test);
}

inline void BattleDefeatRecoveryTest(TestSuite &suite) {
    driveDefeatRecovery(suite, false, "Wild defeat recovery");
    driveDefeatRecovery(suite, true, "Trainer defeat recovery");
}

inline void BattleFlowSuite(TestRunner &runner) {
    TestSuite suite("Slim battle frame controller");
    BattleFlowControllerTest(suite);
    BattleDefeatRecoveryTest(suite);
    runner.addTestSuite(suite);
}
