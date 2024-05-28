#pragma once
#include "test.hpp"

#include "../src/lib/ReadData.hpp"
#include "../src/engine/battle/Battle.hpp"

void EngineTest() {
    Test test;
    BattleEngine eng = BattleEngine();
    eng.startFight(0);

    test.printSummary("Engine load Test");
}

void EngineSuite() {
    printHeader("Engine Suite");
    EngineTest();
}