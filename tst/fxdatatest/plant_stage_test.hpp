#pragma once

#include <string.h>

#include "fxtest.hpp"
#include "src/plants/PlantStage.hpp"
#include "src/plants/PlantGamestate.hpp"

// Device mirror of the host PlantStage suite. The packed byte transform must
// advance four independent two-bit stages in each byte (all 32 stages) with
// saturating growth (empty holds, planted->growing->mature, mature holds) while
// the committed 128-step growth boundary stays unchanged. No generated record
// IDs: this suite operates packed stage bytes and derives its capacities from
// the PlantStage value layout.
namespace plant_stage_fx_test_detail {

// Independent oracle for one packed byte: advance each two-bit slot by one with
// saturating growth, without reusing incrementAll's arithmetic.
inline uint8_t advanceByte(uint8_t packed)
{
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

} // namespace plant_stage_fx_test_detail

inline void test_plant_stage(FxTest &test)
{
    using plant_stage_fx_test_detail::advanceByte;

    const uint8_t byteCount = sizeof(PlantStage);   // eight packed bytes
    const uint8_t slotCount = byteCount * 4;        // thirty-two two-bit stages

    test.expectEq(FIRST_BETA_PLOT_INDEX, static_cast<uint8_t>(0),
                  F("authored first-beta plot is packed plant stage index 0"));

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
        test.expectEq(got[byte], static_cast<uint8_t>(0xAA),
                      F("one pass grows the fully planted shelf"));
    }
    saturate.incrementAll();
    saturate.serialize(got);
    for (uint8_t byte = 0; byte < byteCount; ++byte) {
        test.expectEq(got[byte], static_cast<uint8_t>(0xFF),
                      F("the next pass matures the growing shelf"));
    }
    saturate.incrementAll();
    uint8_t held[8] = {};
    saturate.serialize(held);
    for (uint8_t byte = 0; byte < byteCount; ++byte) {
        test.expectEq(held[byte], static_cast<uint8_t>(0xFF),
                      F("a mature shelf holds instead of wrapping"));
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

    // Committed world-step timing: growth fires only on the 128th tick and moves
    // a planted shelf to growing.
    PlantGameState clocked = PlantGameState();
    for (uint8_t slot = 0; slot < slotCount; ++slot) {
        clocked.plantStages.increment(slot); // plant every slot (0 -> 1)
    }
    for (uint8_t tick = 0; tick < 127; ++tick) {
        clocked.tick();
    }
    test.expectEq(clocked.ticker, 127,
                  F("127 committed ticks stay before the growth boundary"));
    for (uint8_t slot = 0; slot < slotCount; ++slot) {
        test.expectEq(clocked.plantStages.getStage(slot), 1,
                      F("growth does not occur at tick 127"));
    }
    clocked.tick();
    test.expectEq(clocked.ticker, 0, F("128th committed tick resets ticker"));
    for (uint8_t slot = 0; slot < slotCount; ++slot) {
        test.expectEq(clocked.plantStages.getStage(slot), 2,
                      F("128th tick grows every planted slot"));
    }
}
