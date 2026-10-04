#pragma once
#include <cstring>

#include "test.hpp"
#include "../src/plants/PlantStage.hpp"

void plantStageTest(TestSuite &t) {
    Test test = Test(__func__);
    PlantStage stage = PlantStage();
    for (uint8_t i = 0; i < 32; ++i) {
        stage.increment(i);
    }
    for (uint8_t i = 0; i < 32; ++i) {
        test.assert(stage.getStage(i), static_cast<uint8_t>(1),
                    "each index increments");
    }

    for (uint8_t i = 0; i < 32; ++i) {
        stage.increment(i);
        stage.increment(i);
        test.assert(stage.getStage(i), static_cast<uint8_t>(3),
                    "stage reaches three");
        stage.increment(i);
        test.assert(stage.getStage(i), static_cast<uint8_t>(0),
                    "stage wraps three to zero");
    }

    stage.incrementAll();
    for (uint8_t i = 0; i < 32; ++i) {
        test.assert(stage.getStage(i), static_cast<uint8_t>(1),
                    "incrementAll");
    }

    PlantStage encoded = PlantStage();
    const uint8_t pattern[32] = {
        0, 1, 2, 3, 3, 2, 1, 0,
        1, 3, 0, 2, 2, 0, 3, 1,
        3, 1, 2, 0, 0, 2, 1, 3,
        2, 3, 1, 0, 1, 0, 2, 3,
    };
    for (uint8_t i = 0; i < 32; ++i) {
        for (uint8_t count = 0; count < pattern[i]; ++count) {
            encoded.increment(i);
        }
    }
    uint8_t bytes[8] = {};
    std::memcpy(bytes, &encoded, sizeof(bytes));
    // Old uint64_t storage encoded this value little-endian.
    const uint8_t expected[8] = {0xE4, 0x1B, 0x8D, 0x72,
                                 0x27, 0xD8, 0x1E, 0xE1};
    for (uint8_t i = 0; i < 8; ++i) {
        test.assert(bytes[i], expected[i], "save byte layout");
    }
    t.addTest(test);
}

void PlantStageSuite(TestRunner &r) {
    TestSuite t = TestSuite("PlantStage Suite");
    plantStageTest(t);
    r.addTestSuite(t);
}