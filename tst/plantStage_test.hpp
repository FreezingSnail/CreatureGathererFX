#pragma once
#include <cstring>

#include "test.hpp"
#include "../src/plants/PlantStage.hpp"
#include "../src/plants/PlantGamestate.hpp"

namespace plantStageTestDetail {

// Independent oracle for the packed plant-stage byte. Advances each of the four
// two-bit slots by one modulo four without sharing incrementAll's arithmetic, so
// the packed transform can be checked against a per-slot reference.
inline uint8_t advanceByte(uint8_t packed) {
    uint8_t expected = 0;
    for (uint8_t slot = 0; slot < 4; ++slot) {
        const uint8_t shift = static_cast<uint8_t>(slot * 2);
        const uint8_t stage = static_cast<uint8_t>((packed >> shift) & 0x03u);
        expected |= static_cast<uint8_t>(((stage + 1) & 0x03u) << shift);
    }
    return expected;
}

} // namespace plantStageTestDetail

void plantStageTest(TestSuite &t) {
    Test test = Test(__func__);
    using plantStageTestDetail::advanceByte;

    const uint8_t byteCount = sizeof(PlantStage);   // eight packed bytes
    const uint8_t slotCount = byteCount * 4;        // thirty-two two-bit stages

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

    // Exhaustive packed-byte coverage: every one of the 256 byte values at every
    // byte position, with mixed surrounding bytes and packed boundaries, checked
    // against the independent per-slot oracle.
    const uint8_t backgrounds[2][8] = {
        {0x00, 0xFF, 0xAA, 0x55, 0x33, 0xCC, 0x0F, 0xF0},
        {0xFF, 0x00, 0x55, 0xAA, 0xCC, 0x33, 0xF0, 0x0F},
    };
    for (uint8_t backgroundIndex = 0; backgroundIndex < 2; ++backgroundIndex) {
        for (uint8_t position = 0; position < byteCount; ++position) {
            for (uint16_t packed = 0; packed < 256; ++packed) {
                uint8_t input[8] = {};
                for (uint8_t byte = 0; byte < byteCount; ++byte) {
                    input[byte] = byte == position
                                      ? static_cast<uint8_t>(packed)
                                      : backgrounds[backgroundIndex][byte];
                }

                PlantStage candidate = PlantStage();
                candidate.restore(input);
                candidate.incrementAll();
                uint8_t output[8] = {};
                candidate.serialize(output);

                for (uint8_t byte = 0; byte < byteCount; ++byte) {
                    test.assert(output[byte], advanceByte(input[byte]),
                                "all 256 packed byte patterns advance every slot");
                }
            }
        }
    }

    // Full modulo-four cycle: four incrementAll calls restore the original bytes.
    PlantStage cycle = PlantStage();
    cycle.restore(backgrounds[0]);
    uint8_t start[8] = {};
    cycle.serialize(start);
    uint8_t running[8] = {};
    for (uint8_t byte = 0; byte < byteCount; ++byte) {
        running[byte] = backgrounds[0][byte];
    }
    for (uint8_t pass = 0; pass < 4; ++pass) {
        cycle.incrementAll();
        for (uint8_t byte = 0; byte < byteCount; ++byte) {
            running[byte] = advanceByte(running[byte]);
        }
        uint8_t got[8] = {};
        cycle.serialize(got);
        if (pass < 3) {
            for (uint8_t byte = 0; byte < byteCount; ++byte) {
                test.assert(got[byte], running[byte],
                            "incrementAll advances one stage per pass");
            }
        }
    }
    uint8_t finished[8] = {};
    cycle.serialize(finished);
    for (uint8_t byte = 0; byte < byteCount; ++byte) {
        test.assert(finished[byte], start[byte], "four incrementAll calls wrap to start");
    }

    // Standalone increment stays local: only the addressed two-bit field changes.
    for (uint8_t index = 0; index < slotCount; ++index) {
        PlantStage local = PlantStage();
        local.restore(backgrounds[0]);
        local.increment(index);

        const uint8_t *before = backgrounds[0];
        uint8_t after[8] = {};
        local.serialize(after);

        const uint8_t byteIndex = static_cast<uint8_t>(index >> 2);
        const uint8_t shift = static_cast<uint8_t>((index & 0x03u) << 1);
        const uint8_t expectedStage = static_cast<uint8_t>(
            (((before[byteIndex] >> shift) + 1) & 0x03u));
        for (uint8_t byte = 0; byte < byteCount; ++byte) {
            uint8_t expectedByte = before[byte];
            if (byte == byteIndex) {
                const uint8_t mask = static_cast<uint8_t>(0x03u << shift);
                expectedByte = static_cast<uint8_t>((expectedByte & ~mask) |
                                                    (expectedStage << shift));
            }
            test.assert(after[byte], expectedByte, "standalone increment stays local");
        }
    }

    // Committed world-step timing: growth fires only on the 128th tick.
    PlantGameState clocked = PlantGameState();
    for (uint8_t tick = 0; tick < 127; ++tick) {
        clocked.tick();
    }
    test.assert(clocked.ticker, static_cast<uint8_t>(127),
                "127 committed ticks stay before the growth boundary");
    for (uint8_t slot = 0; slot < slotCount; ++slot) {
        test.assert(clocked.plantStages.getStage(slot), static_cast<uint8_t>(0),
                    "growth does not occur at tick 127");
    }
    clocked.tick();
    test.assert(clocked.ticker, static_cast<uint8_t>(0),
                "128th committed tick resets ticker");
    for (uint8_t slot = 0; slot < slotCount; ++slot) {
        test.assert(clocked.plantStages.getStage(slot), static_cast<uint8_t>(1),
                    "128th tick advances every plant slot");
    }

    t.addTest(test);
}

void PlantStageSuite(TestRunner &r) {
    TestSuite t = TestSuite("PlantStage Suite");
    plantStageTest(t);
    r.addTestSuite(t);
}
