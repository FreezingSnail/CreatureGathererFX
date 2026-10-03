#pragma once

#include "fxtest.hpp"
#include "src/lib/FxRead.hpp"
#include "src/lib/ReadData.hpp"
#include "src/fxdata.h"
#include "src/engine/draw.h"

inline void test_readcounter(FxTest &test) {
    uint8_t row[16];
    FxReadCounter::resetFrame();
    test.expectEq(FxReadCounter::markUpdate(), true, F("idle update has zero reads"));
    test.expectEq(FxReadCounter::count(), static_cast<uint8_t>(0), F("reset count"));

    for (uint8_t i = 0; i < 4; ++i) {
        const uint24_t rowAddress = raw_map_data + static_cast<uint24_t>(i) * 512;
        fx_read_data_bytes(rowAddress, row, sizeof(row));
    }
    test.expectEq(FxReadCounter::count(), static_cast<uint8_t>(4), F("four row blocks"));
    test.expectEq(FxReadCounter::renderExact(4), true, F("four row budget"));
    test.expectEq(FxReadCounter::renderExact(5), false, F("four is not five"));

    fx_read_data_bytes(raw_map_data + static_cast<uint24_t>(4) * 512, row, sizeof(row));
    test.expectEq(FxReadCounter::renderExact(4), false, F("extra row fails exact four"));
    test.expectEq(FxReadCounter::renderExact(5), true, F("five row budget"));

    FxReadCounter::resetFrame();
    FxRead::indexed24(CreatureNames::CreatureNames, 0);
    test.expectEq(FxReadCounter::markUpdate(), false, F("lookup fails update budget"));

    FxReadCounter::resetFrame();
    FxRead::indexed24(CreatureNames::CreatureNames, 0);
    test.expectEq(FxReadCounter::transitionExact(1), true, F("table transition exact"));
    test.expectEq(FxReadCounter::markUpdate(), true, F("transition leaves zero update"));

    FxReadCounter::resetFrame();
    readCreatureNameAddress(0);
    test.expectEq(FxReadCounter::count(), static_cast<uint8_t>(1),
                  F("read from another translation unit counted"));
}
