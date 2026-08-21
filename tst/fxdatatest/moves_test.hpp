#pragma once

#include <avr/pgmspace.h>
#include <string.h>

#include "fxtest.hpp"
#include "move_data.hpp"
#include "src/lib/ReadData.hpp"

/*
 * Empirical Ardens capture: generated moves.bin begins 05 00 FF FF and
 * readMoveFX(0) decodes 0x0500FFFF; all 44 records pass, confirming that
 * readIndexedUInt32 assembles this table most-significant-byte first. The
 * distinct records also confirm readMoveFX uses the required four-byte stride
 * (move_table + 4 * index). Any future byte-order or stride mismatch remains
 * a deterministic failure rather than being corrected in src.
 */
void test_moves(FxTest &test) {
    for (uint8_t index = 0; index < moveFixtureCount; ++index) {
        const uint32_t raw = pgm_read_dword(&moveFixtures[index]);
        MoveFixtureExpectation expected;
        memcpy_P(&expected, &moveFixtureExpectations[index], sizeof(expected));

        Move move = readMoveFX(index);
        test.expectEqIdx(move.move, raw >> 16, F("move"), index);
        test.expectEqIdx(move.getMoveType(), expected.type, F("type"), index);
        test.expectEqIdx(move.getMovePower(), expected.power, F("power"), index);
        test.expectEqIdx(move.isPhysical(), expected.physical, F("physical"), index);
        test.expectEqIdx(static_cast<uint8_t>(move.getMoveAccuracy()), expected.accuracy,
                         F("accuracy"), index);
        test.expectEqIdx(static_cast<uint8_t>(move.effect1), expected.effect1,
                         F("effect1"), index);
        test.expectEqIdx(static_cast<uint8_t>(move.effect2), expected.effect2,
                         F("effect2"), index);
    }
}
