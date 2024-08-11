#pragma once
#include "test.hpp"

#include "../src/lib/ReadData.hpp"
#include "../src/engine/battle/Battle.hpp"
#include "../src/globals.hpp"
#include "src/helpers.hpp"

void reset() {
    player = Player();
    battleEventPlayer = BattleEventPlayer();
    menuStack = MenuStack();
    dialogMenu = DialogMenu();
}

void EngineStartTest(TestSuite &t) {
    Test test = Test(__func__);
    BattleEngine eng = BattleEngine();
    eng.startFight(0);
    test.assert(eng.activeBattle, true, "Active Battle");
    test.assert(eng.playerAction.actionIndex, -1, "Player Action Index not set");

    t.addTest(test);
}

void EngineQueueActionTest(TestSuite &t) {
    reset();
    Test test = Test(__func__);
    BattleEngine eng = BattleEngine();
    eng.startFight(0);
    eng.queueAction(ActionType::ATTACK, 0);
    player.basic();
    eng.opponentCur->level = 31;
    eng.opponentHealths[0] = 1000;
    test.assert(eng.PlayerActionReady(), true, "Action Queued");
    test.assert(eng.playerAction.actionIndex, 0, "Action Index set");
    test.assert(eng.playerAction.actionType, ActionType::ATTACK, "Action Type set");
    test.assert(eng.playerAction.priority, Priority::NORMAL, "Action priority set");

    t.addTest(test);
}

void EngineTickTest(TestSuite &t) {
    reset();
    Test test = Test(__func__);
    BattleEngine eng = BattleEngine();
    player.basic();
    eng.startFight(0);
    test.assert(battleEventPlayer.stackPointer, -1, "battleEventPlayer.stackPointer Pointer empty");
    test.addToLog("Healths: " + std::to_string(eng.playerHealths[0]) + " " + std::to_string(eng.opponentHealths[0]));
    // TODO: shoudnt need to bump opponent for this test
    eng.opponentCur->level = 31;
    eng.opponentHealths[0] = 1000;
    test.addToLog("Healths: " + std::to_string(eng.playerHealths[0]) + " " + std::to_string(eng.opponentHealths[0]));

    auto playerHP = eng.playerHealths[0];
    auto opponentHP = eng.opponentHealths[0];

    test.assert(eng.activeBattle, true, "Active Battle");
    test.assert(eng.playerAction.actionIndex, -1, "Player Action Index not set");

    eng.queueAction(ActionType::ATTACK, 0);

    eng.turnState = BattleState::TURN_INPUT;
    eng.turnTick();

    eng.turnState = BattleState::PLAYER_ATTACK;
    eng.turnTick();
    test.addToLog("battleEventPlayer.stackPointer: " + std::to_string(battleEventPlayer.stackPointer));
    for (int i = 0; i <= battleEventPlayer.stackPointer; i++) {
        test.addToLog("battleEventStack " + std::to_string(i) + " Type: " + BattleEventTypeToString(battleEventStack[i].type));
        test.addToLog("battleEventStack " + std::to_string(i) + " Index: " + std::to_string(static_cast<int>(battleEventStack[i].index)));
        test.addToLog("battleEventStack " + std::to_string(i) + " Data: " + std::to_string(static_cast<int>(battleEventStack[i].data)));
    }
    test.assert(battleEventPlayer.stackPointer, 0, "battleEventPlayer.stackPointer Pointer set");
    test.assert(battleEventStack[0].type, BattleEventType::ATTACK, "battleEventStack 0 Type set");
    test.assert(battleEventStack[0].index, eng.playerCur->id, "battleEventStack 0 index set");

    eng.turnState = BattleState::OPPONENT_RECEIVE_DAMAGE;
    eng.turnTick();
    test.assert(battleEventStack[1].type, BattleEventType::OPPONENT_DAMAGE, "battleEventStack 1 Type set");
    test.assert(battleEventStack[1].index, 0, "DibattleEventStackaaddToLog 1 index set");
    test.assertGreaterThan(battleEventStack[1].data, 0, "battleEventStack 1 data set");

    eng.turnState = BattleState::OPPONENT_RECEIVE_EFFECT_APPLICATION;
    eng.turnTick();

    eng.turnState = BattleState::OPPONENT_ATTACK;
    eng.turnTick();
    test.assert(battleEventStack[2].type, BattleEventType::OPPONENT_ATTACK, "battleEventStack 2 Type set");
    test.assert(battleEventStack[2].index, eng.opponentCur->id, "battleEventStack 2 index set");

    eng.turnState = BattleState::PLAYER_RECEIVE_DAMAGE;
    eng.turnTick();
    test.assert(battleEventStack[3].type, BattleEventType::DAMAGE, "battleEventStack 3 Type set");
    test.assert(battleEventStack[3].index, 0, "battleEventStack 3 index set");
    test.assertGreaterThan(battleEventStack[3].data, 0, "battleEventStack 3 data set");

    eng.turnState = BattleState::PLAYER_RECEIVE_EFFECT_APPLICATION;
    eng.turnTick();

    eng.turnState = BattleState::END_TURN;
    eng.turnTick();

    test.assert(eng.playerHealths[0], playerHP - battleEventStack[3].data, "Player Health set");
    test.assert(eng.opponentHealths[0], opponentHP - battleEventStack[1].data, "Opponent Health set");

    t.addTest(test);
}

void EngineSuite(TestRunner &r) {
    TestSuite t = TestSuite("Engine Suite");
    EngineStartTest(t);
    EngineQueueActionTest(t);
    EngineTickTest(t);
    r.addTestSuite(t);
    reset();
}
