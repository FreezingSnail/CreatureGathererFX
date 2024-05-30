#pragma once
#include "test.hpp"

#include "../src/lib/ReadData.hpp"
#include "../src/engine/battle/Battle.hpp"

void EngineTest(TestSuite &t) {
    Test test = Test(__func__);
    BattleEngine eng = BattleEngine();
    eng.startFight(0);

    t.addTest(test);
}

void EngineSuite(TestRunner &r) {
    TestSuite t = TestSuite("Engine Suite");
    EngineTest(t);
    r.addTestSuite(t);
}