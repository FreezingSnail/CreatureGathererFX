#pragma once

#include <string.h>

#include "fxtest.hpp"
#include "src/fxdata.h"
#include "src/lib/ReadData.hpp"
#include "generated/rawread_data.hpp"

// Lean sketch linker globals: `arduboy` and `dialogMenu` only. DialogMenu is
// retained because its source is compiled; no game-state, menu, world, or
// battle globals are needed by ReadFXu16() or FX::readDataObject().
inline void test_rawread(FxTest &test) {
    const uint16_t first = ReadFXu16(move_table);
    const uint16_t repeated = ReadFXu16(move_table);
    const uint16_t next = ReadFXu16(move_table + sizeof(uint16_t));

    test.expectEq(first, rawReadMoveTableFirst, F("boundary little endian"));
    test.expectEq(first, repeated, F("cursor reset"));
    test.expectEq(next, rawReadMoveTableNext, F("sequential window"));

    OpponentSeed before;
    OpponentSeed after;
    FX::readDataObject(opponent_seeds, before);
    ReadFXu16(move_table + sizeof(uint16_t));
    FX::readDataObject(opponent_seeds, after);

    test.expectEq(memcmp(&before, &rawReadOpponentSeed, sizeof(before)) == 0,
                  true, F("object before raw read"));
    test.expectEq(memcmp(&after, &rawReadOpponentSeed, sizeof(after)) == 0,
                  true, F("object after raw read"));
    test.expectEq(memcmp(&after, &before, sizeof(after)) == 0,
                  true, F("object api interop"));
}
