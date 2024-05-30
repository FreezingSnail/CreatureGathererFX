#pragma once
#include "test.hpp"
#include "../src/plants/PlantStage.hpp"

void plantStageTest(TestSuite &t) {
    Test test = Test(__func__);
    PlantStage stage = PlantStage();
    test.assert(int(stage.getStage(0)), 0, "getStage");
    stage.increment(0);
    test.assert(int(stage.getStage(0)), 1, "getStage");
    stage.increment(0);
    test.assert(int(stage.getStage(0)), 2, "getStage");
    stage.incrementAll();
    for (uint8_t i = 1; i < 32; i++) {
        test.assert(int(stage.getStage(i)), 1, "getStage");
    }
    t.addTest(test);
}

void PlantStageSuite(TestRunner &r) {
    TestSuite t = TestSuite("PlantStage Suite");
    plantStageTest(t);
    r.addTestSuite(t);
}