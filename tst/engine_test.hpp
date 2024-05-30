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
    eng.opponentHealths[0] = 100;
    test.assert(eng.queued, true, "Action Queued");
    test.assert(eng.playerAction.actionIndex, 0, "Action Index set");
    test.assert(eng.playerAction.actionType, ActionType::ATTACK, "Action Type set");
    test.assert(eng.playerAction.priority, Priority::NORMAL, "Action priority set");
    eng.turnTick();
    test.assert(eng.queued, false, "Action Dequeued");

    t.addTest(test);
}

void EngineTickTest(TestSuite &t) {
    reset();
    Test test = Test(__func__);
    BattleEngine eng = BattleEngine();
    player.basic();
    eng.startFight(0);
    test.assert(battleEventPlayer.stackPointer, -1, "battleEventPlayer.stackPointer Pointer empty");
    std::cout << "Healths: " << eng.playerHealths[0] << " " << eng.opponentHealths[0] << std::endl;
    // TODO: shoudnt need to bump opponent for this test
    eng.opponentCur->level = 31;
    eng.opponentHealths[0] = 100;

    auto playerHP = eng.playerHealths[0];
    auto opponentHP = eng.opponentHealths[0];

    test.assert(eng.activeBattle, true, "Active Battle");
    test.assert(eng.playerAction.actionIndex, -1, "Player Action Index not set");
    eng.queueAction(ActionType::ATTACK, 0);
    eng.turnTick();
    std::cout << "Healths: " << eng.playerHealths[0] << " " << eng.opponentHealths[0] << std::endl;

    std::cout << "battleEventPlayer.stackPointer: " << battleEventPlayer.stackPointer << std::endl;
    for (int i = 0; i <= battleEventPlayer.stackPointer; i++) {
        std::cout << "battleEventStack " << i << " Type: " << BattleEventTypeToString(battleEventStack[i].type) << std::endl;
        std::cout << "battleEventStack " << i << " Index: " << static_cast<int>(battleEventStack[i].index) << std::endl;
        std::cout << "battleEventStack " << i << " Data: " << static_cast<int>(battleEventStack[i].data) << std::endl;
    }
    test.assert(battleEventPlayer.stackPointer, 3, "battleEventPlayer.stackPointer Pointer set");
    test.assert(battleEventStack[0].type, BattleEventType::ATTACK, "battleEventStack 0 Type set");
    test.assert(battleEventStack[0].index, eng.playerCur->id, "battleEventStack 0 index set");
    test.assert(battleEventStack[1].type, BattleEventType::DAMAGE, "battleEventStack 1 Type set");
    test.assert(battleEventStack[1].index, 0, "DibattleEventStackalog 1 index set");
    test.assertGreaterThan(battleEventStack[1].data, 0, "battleEventStack 1 data set");
    test.assert(battleEventStack[2].type, BattleEventType::OPPONENT_ATTACK, "battleEventStack 2 Type set");
    test.assert(battleEventStack[2].index, eng.opponentCur->id, "battleEventStack 2 index set");
    test.assert(battleEventStack[3].type, BattleEventType::OPPONENT_DAMAGE, "battleEventStack 3 Type set");
    test.assert(battleEventStack[3].index, 0, "battleEventStack 3 index set");
    test.assertGreaterThan(battleEventStack[3].data, 0, "battleEventStack 3 data set");

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
