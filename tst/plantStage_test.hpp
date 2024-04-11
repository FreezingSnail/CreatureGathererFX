#pragma once
#include "test.hpp"
#include "../src/plants/PlantStage.hpp"

void plantStageTest() {
    Test test;
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
    test.printSummary("plantStageTest");
}

void PlantStageSuite() {
    printHeader("PlantStage Suite");
    plantStageTest();
}