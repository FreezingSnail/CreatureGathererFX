#pragma once

#include <string.h>

#include "fxtest.hpp"
#include "src/fxdata.h"
#include "src/lib/ReadData.hpp"

/*
 * Read-path contracts for the generated tables, asserted on hardware because
 * the byte order depends on which ArduboyFX helper is used:
 *
 *   - FX::readIndexedUInt16/24/32 and FX::readPending*  assemble
 *     most-significant byte first, which is what the packer writes for typed
 *     symbol tables (uint24_t address tables, packed move rows).
 *   - FX::readDataObject/readDataBytes copy raw bytes, so multi-byte struct
 *     fields and the script text block are little endian.
 *
 * Mixing the two silently byte-swaps. These tests pin each convention to the
 * table it belongs to, and pin the one ArduboyFX helper that is outright broken
 * (readIndexedUInt32 seeks with a three-byte stride), so a library upgrade that
 * changes either behaviour fails here instead of in game logic.
 */

namespace {

uint32_t composeBigEndian(const uint8_t *bytes, uint8_t length) {
    uint32_t value = 0;
    for (uint8_t index = 0; index < length; ++index) {
        value = (value << 8) | bytes[index];
    }
    return value;
}

uint16_t composeLittleEndian16(const uint8_t *bytes) {
    return static_cast<uint16_t>(bytes[1]) << 8 | bytes[0];
}

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
    const uint24_t actual = FX::readIndexedUInt24(table, index);
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

inline void test_address_table(FxTest &test) {
    const uint24_t expected[] = {
        MoveData::move0, MoveData::move1, MoveData::move2,
        MoveData::move3, MoveData::move4, MoveData::move5,
    };
    const __FlashStringHelper *const raw_labels[] = {
        F("moveNames uint24 byte 0/raw byte 2"), F("moveNames uint24 byte 1/raw byte 1"),
        F("moveNames uint24 byte 2/raw byte 0"),
    };
    const __FlashStringHelper *const published_labels[] = {
        F("moveNames published byte 0"), F("moveNames published byte 1"),
        F("moveNames published byte 2"),
    };

    for (uint8_t index = 0; index < sizeof(expected) / sizeof(expected[0]); ++index) {
        expect_indexed_uint24_bytes(test, MoveData::moveNames, index, expected[index], raw_labels,
                                    published_labels);
    }
}

// CreatureNames is at 0x04E9D2; its entries (creature0 through creature31)
// are above 0x010000. This exercises the uint24_t high byte without widening.
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

inline void test_indexed_bytes(FxTest &test) {
    uint8_t raw[8];
    FX::readDataBytes(type_table, raw, sizeof(raw));
    for (uint8_t index = 0; index < sizeof(raw); ++index) {
        test.expectEqIdx(FX::readIndexedUInt8(type_table, index), raw[index],
                         F("type_table byte"), index);
    }
}

// ArduboyFX seeks readIndexedUInt32 with sizeof(uint24_t), so index 1 lands
// three bytes in rather than four. readMoveFX() avoids it by computing the row
// address itself; this pins the defect so the workaround is not dropped early.
inline void test_indexed_u32_stride(FxTest &test) {
    uint8_t raw[4];
    FX::readDataBytes(move_table + 3, raw, sizeof(raw));
    test.expectEq(FX::readIndexedUInt32(move_table, 1), composeBigEndian(raw, sizeof(raw)),
                  F("readIndexedUInt32 uses a three byte stride"));

    uint8_t row[4];
    FX::readDataBytes(move_table + 4, row, sizeof(row));
    test.expectEq(readMoveFX(1).move, composeBigEndian(row, sizeof(row)) >> 16,
                  F("readMoveFX uses a four byte stride"));
}

// The script text block is little endian: u16 count, u16 offset per string,
// then each string as u16 length followed by its bytes.
inline void test_text_block(FxTest &test) {
    uint8_t header[4];
    FX::readDataBytes(raw_map_text, header, sizeof(header));
    const uint16_t count = composeLittleEndian16(&header[0]);

    test.expectEq(ReadFXu16(raw_map_text), count, F("text count little endian"));
    test.expectEq(count > 0 && count <= 4096, true, F("text count plausible"));
    test.expectEq(FX::readIndexedUInt16(raw_map_text, 0),
                  static_cast<uint16_t>(count << 8 | count >> 8),
                  F("indexed u16 byte swaps the count"));

    // First offset is always zero, and every offset ascends.
    test.expectEq(ReadFXu16(raw_map_text + 2), 0, F("first text offset"));
    const uint24_t blobStart = raw_map_text + 2 + (2 * static_cast<uint24_t>(count));
    uint16_t previous = 0;
    for (uint16_t index = 1; index < count; ++index) {
        const uint16_t offset = ReadFXu16(raw_map_text + 2 + (2 * index));
        test.expectEqIdx(offset > previous, true, F("text offset ascends"),
                         static_cast<uint8_t>(index));
        const uint16_t length = ReadFXu16(blobStart + offset);
        test.expectEqIdx(length > 0 && length <= 1024, true, F("text length plausible"),
                         static_cast<uint8_t>(index));
        previous = offset;
    }
}

inline void test_tables(FxTest &test) {
    test_address_table(test);
    test_high_address_table(test);
    test_indexed_bytes(test);
    test_indexed_u32_stride(test);
    test_text_block(test);
}
