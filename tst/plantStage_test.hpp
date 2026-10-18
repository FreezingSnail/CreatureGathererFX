#pragma once
#include <cstring>

#include "test.hpp"
#include "../src/plants/PlantStage.hpp"
#include "../src/plants/PlantGamestate.hpp"

namespace plantStageTestDetail {

// Independent oracle for the packed plant-stage byte. Advances each of the four
// two-bit slots by one with saturating growth, without sharing incrementAll's
// arithmetic: an empty slot stays empty, planted->growing->mature and a mature
// slot holds.
inline uint8_t advanceByte(uint8_t packed) {
    uint8_t expected = 0;
    for (uint8_t slot = 0; slot < 4; ++slot) {
        const uint8_t shift = static_cast<uint8_t>(slot * 2);
        const uint8_t stage = static_cast<uint8_t>((packed >> shift) & 0x03u);
        uint8_t next = 0;
        if (stage == 1) next = 2;
        else if (stage >= 2) next = 3;
        expected |= static_cast<uint8_t>(next << shift);
    }
    return expected;
}

} // namespace plantStageTestDetail

void plantStageTest(TestSuite &t) {
    Test test = Test(__func__);
    using plantStageTestDetail::advanceByte;

    const uint8_t byteCount = sizeof(PlantStage);   // eight packed bytes
    const uint8_t slotCount = byteCount * 4;        // thirty-two two-bit stages

    test.assert(FIRST_BETA_PLOT_INDEX, static_cast<uint8_t>(0),
                "authored first-beta plot is packed plant stage index 0");

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
        test.assert(stage.getStage(i), static_cast<uint8_t>(0),
                    "incrementAll leaves an empty stage empty");
    }
    for (uint8_t i = 0; i < 32; ++i) {
        stage.increment(i); // plant: 0 -> 1
    }
    stage.incrementAll();
    for (uint8_t i = 0; i < 32; ++i) {
        test.assert(stage.getStage(i), static_cast<uint8_t>(2),
                    "incrementAll grows a planted stage");
    }
    stage.incrementAll();
    for (uint8_t i = 0; i < 32; ++i) {
        test.assert(stage.getStage(i), static_cast<uint8_t>(3),
                    "incrementAll matures a growing stage");
    }
    stage.incrementAll();
    for (uint8_t i = 0; i < 32; ++i) {
        test.assert(stage.getStage(i), static_cast<uint8_t>(3),
                    "incrementAll holds a mature stage instead of wrapping");
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

    // Saturating growth from a fully planted shelf: one pass grows every slot to
    // growing (0x55 -> 0xAA), the next matures it (0xAA -> 0xFF), and a mature
    // shelf holds byte-for-byte under further passes.
    PlantStage saturate = PlantStage();
    const uint8_t planted[8] = {0x55, 0x55, 0x55, 0x55,
                                0x55, 0x55, 0x55, 0x55};
    saturate.restore(planted);
    uint8_t got[8] = {};
    saturate.incrementAll();
    saturate.serialize(got);
    for (uint8_t byte = 0; byte < byteCount; ++byte) {
        test.assert(got[byte], static_cast<uint8_t>(0xAA),
                    "one pass grows the fully planted shelf");
    }
    saturate.incrementAll();
    saturate.serialize(got);
    for (uint8_t byte = 0; byte < byteCount; ++byte) {
        test.assert(got[byte], static_cast<uint8_t>(0xFF),
                    "the next pass matures the growing shelf");
    }
    saturate.incrementAll();
    uint8_t held[8] = {};
    saturate.serialize(held);
    for (uint8_t byte = 0; byte < byteCount; ++byte) {
        test.assert(held[byte], static_cast<uint8_t>(0xFF),
                    "a mature shelf holds instead of wrapping");
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

    // Committed world-step timing: growth fires only on the 128th tick and moves
    // a planted shelf to growing.
    PlantGameState clocked = PlantGameState();
    for (uint8_t slot = 0; slot < slotCount; ++slot) {
        clocked.plantStages.increment(slot); // plant every slot (0 -> 1)
    }
    for (uint8_t tick = 0; tick < 127; ++tick) {
        clocked.tick();
    }
    test.assert(clocked.ticker, static_cast<uint8_t>(127),
                "127 committed ticks stay before the growth boundary");
    for (uint8_t slot = 0; slot < slotCount; ++slot) {
        test.assert(clocked.plantStages.getStage(slot), static_cast<uint8_t>(1),
                    "growth does not occur at tick 127");
    }
    clocked.tick();
    test.assert(clocked.ticker, static_cast<uint8_t>(0),
                "128th committed tick resets ticker");
    for (uint8_t slot = 0; slot < slotCount; ++slot) {
        test.assert(clocked.plantStages.getStage(slot), static_cast<uint8_t>(2),
                    "128th tick grows every planted slot");
    }

    t.addTest(test);
}

void PlantStageSuite(TestRunner &r) {
    TestSuite t = TestSuite("PlantStage Suite");
    plantStageTest(t);
    r.addTestSuite(t);
}
