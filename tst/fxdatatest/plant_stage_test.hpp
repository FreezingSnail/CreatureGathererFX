#pragma once

#include <string.h>

#include "fxtest.hpp"
#include "src/plants/PlantStage.hpp"
#include "src/plants/PlantGamestate.hpp"

// Device mirror of the host PlantStage suite. The packed byte transform must
// advance four independent two-bit stages in each byte (all 32 stages) while the
// committed 128-step growth boundary stays unchanged. No generated record IDs:
// this suite operates packed stage bytes and derives its capacities from the
// PlantStage value layout.
namespace plant_stage_fx_test_detail {

// Independent oracle for one packed byte: advance each two-bit slot by one
// modulo four, without reusing incrementAll's arithmetic.
inline uint8_t advanceByte(uint8_t packed)
{
    uint8_t expected = 0;
    for (uint8_t slot = 0; slot < 4; ++slot) {
        const uint8_t shift = static_cast<uint8_t>(slot * 2);
        const uint8_t stage = static_cast<uint8_t>((packed >> shift) & 0x03u);
        expected |= static_cast<uint8_t>(((stage + 1) & 0x03u) << shift);
    }
    return expected;
}

} // namespace plant_stage_fx_test_detail

inline void test_plant_stage(FxTest &test)
{
    using plant_stage_fx_test_detail::advanceByte;

    const uint8_t byteCount = sizeof(PlantStage);   // eight packed bytes
    const uint8_t slotCount = byteCount * 4;        // thirty-two two-bit stages

    // Exhaustive packed-byte coverage: every one of the 256 byte values at every
    // byte position, with mixed surrounding bytes and packed boundaries.
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

                PlantStage stage = PlantStage();
                stage.restore(input);
                stage.incrementAll();
                uint8_t output[8] = {};
                stage.serialize(output);

                for (uint8_t byte = 0; byte < byteCount; ++byte) {
                    test.expectEq(output[byte], advanceByte(input[byte]),
                                  F("all 256 packed byte patterns advance every slot"));
                }
            }
        }
    }

    // Full modulo-four cycle: four incrementAll calls restore the original bytes.
    PlantStage cycle = PlantStage();
    cycle.restore(backgrounds[0]);
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
        for (uint8_t byte = 0; byte < byteCount; ++byte) {
            test.expectEq(got[byte], running[byte],
                          F("incrementAll advances one stage per pass"));
        }
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
            test.expectEq(after[byte], expectedByte,
                          F("standalone increment stays local"));
        }
    }

    // Committed world-step timing: growth fires only on the 128th tick.
    PlantGameState clocked = PlantGameState();
    for (uint8_t tick = 0; tick < 127; ++tick) {
        clocked.tick();
    }
    test.expectEq(clocked.ticker, 127,
                  F("127 committed ticks stay before the growth boundary"));
    for (uint8_t slot = 0; slot < slotCount; ++slot) {
        test.expectEq(clocked.plantStages.getStage(slot), 0,
                      F("growth does not occur at tick 127"));
    }
    clocked.tick();
    test.expectEq(clocked.ticker, 0, F("128th committed tick resets ticker"));
    for (uint8_t slot = 0; slot < slotCount; ++slot) {
        test.expectEq(clocked.plantStages.getStage(slot), 1,
                      F("128th tick advances every plant slot"));
    }
}
