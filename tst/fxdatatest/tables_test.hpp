#pragma once

#include <string.h>

#include "fxtest.hpp"
#include "src/fxdata.h"
#include "src/lib/FxRead.hpp"

/*
 * Device read-path contracts for the packed address tables.
 *
 * ArduboyFX 1.4.0's AVR readIndexedUInt24/readPendingLastUInt24 path loses the
 * top table byte. FxRead::indexed24 reads the same packed big-endian table as
 * raw bytes and reconstructs uint24_t, keeping the cart's full 24-bit address.
 * CreatureNames sits at 0x0210AC, so its entries exercise that high byte on
 * real hardware; this suite pins the workaround so it cannot be dropped early.
 *
 * The retired packed-Move battle owned the old move-table/menu-run suites.
 * Their generic FX-read coverage survives elsewhere (ReadFXu16/readDataObject
 * in tiles_test, the 8x8 type chart on host) and is not restated here.
 */

namespace {

// AVR-GCC leaves the fourth byte undefined when __uint24 widens for an
// expectEq* call. Keep every uint24_t as its three-byte object representation;
// only individual uint8_t bytes reach the test API.
inline void expect_indexed_uint24_bytes(
    FxTest &test,
    uint24_t table,
    uint8_t index,
    const uint24_t &published,
    const __FlashStringHelper *const raw_labels[3],
    const __FlashStringHelper *const published_labels[3]) {
    const uint24_t actual = FxRead::indexed24(table, index);
    uint8_t got[3];
    memcpy(got, &actual, sizeof(actual));

    uint8_t want[3];
    FX::readDataBytes(table + 3 * index, want, 3);

    uint8_t published_bytes[3];
    memcpy(published_bytes, &published, sizeof(published));

    for (uint8_t byte = 0; byte < 3; ++byte) {
        // FX reassembles the big-endian table bytes into AVR's little-endian
        // uint24_t object representation, so source byte 2 becomes object byte 0.
        test.expectEqIdx(got[byte], want[2 - byte], raw_labels[byte], index);
        test.expectEqIdx(got[byte], published_bytes[byte], published_labels[byte], index);
    }
}

} // namespace

// CreatureNames is at 0x0210AC; its entries (creature0 through creature31) are
// above 0x010000. This exercises the uint24_t high byte without widening.
inline void test_high_address_table(FxTest &test) {
    const uint24_t expected[] = {
        creature0, creature1, creature2, creature3, creature4, creature5,
        creature6, creature7, creature8, creature9, creature10, creature11,
        creature12, creature13, creature14, creature15, creature16, creature17,
        creature18, creature19, creature20, creature21, creature22, creature23,
        creature24, creature25, creature26, creature27, creature28, creature29,
        creature30, creature31,
    };
    const __FlashStringHelper *const raw_labels[] = {
        F("CreatureNames uint24 byte 0/raw byte 2"),
        F("CreatureNames uint24 byte 1/raw byte 1"),
        F("CreatureNames uint24 byte 2/raw byte 0"),
    };
    const __FlashStringHelper *const published_labels[] = {
        F("CreatureNames published byte 0"), F("CreatureNames published byte 1"),
        F("CreatureNames published byte 2"),
    };

    for (uint8_t index = 0; index < sizeof(expected) / sizeof(expected[0]); ++index) {
        expect_indexed_uint24_bytes(test, CreatureNames::CreatureNames, index, expected[index],
                                    raw_labels, published_labels);
    }
}

// Direct raw-vs-published byte check for the first, middle and last entries,
// plus the high-byte guard that makes those entries exercise byte 2 of the
// uint24 rather than a tautological zero.
inline void test_high_address_raw_bytes(FxTest &test) {
    const uint8_t indices[] = { 0, 15, 31 };
    const uint24_t expected[] = { creature0, creature15, creature31 };

    for (uint8_t sample = 0; sample < sizeof(indices) / sizeof(indices[0]); ++sample) {
        const uint8_t index = indices[sample];
        uint8_t want[3];
        FX::readDataBytes(CreatureNames::CreatureNames + 3 * index, want, 3);
        // Big-endian source byte 0 is the address high byte: nonzero means the
        // entry is above 0x00FFFF (every creature entry here is above 0x010000).
        test.expectEqIdx(want[0] != 0, true, F("CreatureNames raw high byte present"), index);

        uint8_t published[3];
        memcpy(published, &expected[sample], sizeof(published));
        test.expectEqIdx(want[0], published[2], F("CreatureNames raw byte 0 vs published byte 2"),
                         index);
        test.expectEqIdx(want[1], published[1], F("CreatureNames raw byte 1 vs published byte 1"),
                         index);
        test.expectEqIdx(want[2], published[0], F("CreatureNames raw byte 2 vs published byte 0"),
                         index);
    }
}

// readIndexedUInt8 is a single-byte read, so it has no byte-order hazard; the
// pinned contract is that it returns the packed byte for its index.
inline void test_indexed_bytes(FxTest &test) {
    uint8_t raw[8];
    FX::readDataBytes(type_table, raw, sizeof(raw));
    for (uint8_t index = 0; index < sizeof(raw); ++index) {
        test.expectEqIdx(FX::readIndexedUInt8(type_table, index), raw[index],
                         F("type_table byte"), index);
    }
}

inline void test_tables(FxTest &test) {
    test_high_address_table(test);
    test_high_address_raw_bytes(test);
    test_indexed_bytes(test);
}
