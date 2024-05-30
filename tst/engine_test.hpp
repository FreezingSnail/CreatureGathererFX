#pragma once
#include "test.hpp"

#include "../src/lib/ReadData.hpp"
#include "../src/engine/battle/Battle.hpp"

void EngineTest(TestSuite &t) {
    Test test = Test(__func__);
    BattleEngine eng = BattleEngine();
    eng.startFight(0);
    test.assert(eng.activeBattle, true, "Active Battle");
    test.assert(eng.playerAction.actionIndex, -1, "Player Action Index not set");

    t.addTest(test);
}

void EngineSuite(TestRunner &r) {
    TestSuite t = TestSuite("Engine Suite");
    EngineTest(t);
    r.addTestSuite(t);
}