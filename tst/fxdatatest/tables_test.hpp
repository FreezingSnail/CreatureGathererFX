#pragma once

#include <string.h>

#include "fxtest.hpp"
#include "src/fxdata.h"
#include "src/lib/ReadData.hpp"
#include "src/engine/world/Chunk.hpp"
#include "src/vm/opcodes.hpp"

/*
 * Read-path contracts for the generated tables, asserted on hardware because
 * the byte order depends on which ArduboyFX helper is used:
 *
 *   - Indexed FX helpers assemble most-significant byte first, except the AVR
 *     readPendingLastUInt24 path loses the top byte. FxRead::indexed24 reads
 *     the same packed big-endian table as raw bytes and reconstructs uint24_t.
 *   - FX::readDataObject/readDataBytes copy raw bytes, so multi-byte struct
 *     fields and the script text block are little endian.
 *
 * Mixing the two silently byte-swaps. These tests pin each convention to the
 * table it belongs to, and pin the one ArduboyFX helper that is outright broken
 * (readIndexedUInt32 seeks with a three-byte stride), so the workaround stays
 * pinned here instead of depending on the library implementation.
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
    test.expectEq(count <= 4096, true, F("text count plausible"));
    test.expectEq(FX::readIndexedUInt16(raw_map_text, 0),
                  static_cast<uint16_t>(count << 8 | count >> 8),
                  F("indexed u16 byte swaps the count"));

    // A map with only teleports/flags has a canonical zero count and no
    // offsets. Non-empty blocks use the framing checks below.
    if (count == 0) {
        return;
    }

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

inline void expect_uint24_bytes(FxTest &test, const uint24_t &actual,
                                const uint24_t &expected,
                                const __FlashStringHelper *label) {
    uint8_t got[3], want[3];
    memcpy(got, &actual, sizeof(got));
    memcpy(want, &expected, sizeof(want));
    for (uint8_t index = 0; index < 3; ++index) {
        test.expectEqIdx(got[index], want[index], label, index);
    }
}

inline void expect_script_prefix(FxTest &test, uint16_t chunkId,
                                 const uint8_t *prefix, uint8_t size) {
    uint8_t bytes[15];
    FX::readDataBytes(Chunk::scriptSlotAddr(scripts, chunkId), bytes, size);
    for (uint8_t index = 0; index < size; ++index) {
        test.expectEqIdx(bytes[index], pgm_read_byte(prefix + index), F("script blob prefix"), index);
    }
}

inline void test_chunk_layout(FxTest &test) {
    // Prefixes are pinned from the three blobs in regenerated scripts.hpp.
    // The device sketch stages src/ and this suite, not generator source files.
    static const uint8_t blob0Prefix[] PROGMEM = {5, 0, 0, 0, 1, 9, 4, 0, 1, 0, 1, 0, 0, 0, 0};
    static const uint8_t blob32Prefix[] PROGMEM = {4, 0, 4, 0, 4, 0, 12, 0, 7};
    static const uint8_t blob33Prefix[] PROGMEM = {4, 0, 12, 0, 7, 0, 4, 0, 4};
    uint8_t mapBytes[2];
    FX::readDataBytes(Chunk::mapChunkAddr(map_data, 0), mapBytes, sizeof(mapBytes));
    test.expectEq(mapBytes[0], 0, F("map chunk 0 first word high"));
    test.expectEq(mapBytes[1], 7, F("map chunk 0 first word low"));
    FX::readDataBytes(Chunk::mapChunkAddr(map_data, 3), mapBytes, sizeof(mapBytes));
    test.expectEq(mapBytes[0], 0, F("map chunk 3 first word high"));
    test.expectEq(mapBytes[1], 0, F("map chunk 3 first word low"));
    FX::readDataBytes(Chunk::mapChunkAddr(map_data, 2047), mapBytes, sizeof(mapBytes));
    test.expectEq(mapBytes[0], 0, F("map chunk 2047 first word high"));
    test.expectEq(mapBytes[1], 0, F("map chunk 2047 first word low"));

    expect_script_prefix(test, 0, blob0Prefix, sizeof(blob0Prefix));
    expect_script_prefix(test, 32, blob32Prefix, sizeof(blob32Prefix));
    expect_script_prefix(test, 33, blob33Prefix, sizeof(blob33Prefix));

    uint8_t tpIf[9];
    FX::readDataBytes(Chunk::scriptSlotAddr(scripts, 32), tpIf, sizeof(tpIf));
    test.expectEq(tpIf[0], static_cast<uint8_t>(VmOpcode::TpIf), F("script opcode symbol"));
    test.expectEq(composeBigEndian(&tpIf[1], 2), 4, F("script x operand is big endian"));
    test.expectEq(composeBigEndian(&tpIf[3], 2), 4, F("script y operand is big endian"));
    test.expectEq(composeBigEndian(&tpIf[5], 2), 12, F("script target x is big endian"));
    test.expectEq(composeBigEndian(&tpIf[7], 2), 7, F("script target y is big endian"));

    const uint16_t emptySlots[] = {3, 35, 64, 67, 96, 99, 2047};
    const uint24_t imageEnd = scripts +
        static_cast<uint24_t>(Chunk::CHUNK_COUNT) * Chunk::SCRIPT_SLOT_BYTES;
    for (uint8_t index = 0; index < sizeof(emptySlots) / sizeof(emptySlots[0]); ++index) {
        const uint16_t id = emptySlots[index];
        const uint24_t address = Chunk::scriptSlotAddr(scripts, id);
        const uint24_t slotEnd = address + Chunk::SCRIPT_SLOT_BYTES;
        test.expectEqIdx(Chunk::validChunkId(id), true, F("script slot id valid"), index);
        test.expectEqIdx(address >= scripts && slotEnd <= imageEnd, true,
                         F("script slot inside image"), index);
        uint8_t byte;
        FX::readDataBytes(address, &byte, 1);
        FX::readDataBytes(slotEnd - 1, &byte, 1);
    }
    const uint24_t lastAddress = scripts + static_cast<uint24_t>(262016UL);
    expect_uint24_bytes(test, Chunk::scriptSlotAddr(scripts, 2047), lastAddress,
                        F("last script slot uint24 bytes"));
    const uint24_t chunk512Address = Chunk::scriptSlotAddr(scripts, 512);
    expect_uint24_bytes(test, chunk512Address, scripts + static_cast<uint24_t>(65536UL),
                        F("chunk 512 script slot uint24 bytes"));
    test.expectEq(chunk512Address + Chunk::SCRIPT_SLOT_BYTES <= imageEnd, true,
                  F("chunk 512 script slot inside image"));
    expect_uint24_bytes(test, Chunk::mapChunkAddr(map_data, 2047),
                        map_data + static_cast<uint24_t>(131008UL),
                        F("last map chunk uint24 bytes"));
}

inline void test_tables(FxTest &test) {
    test_address_table(test);
    test_high_address_table(test);
    test_indexed_bytes(test);
    test_indexed_u32_stride(test);
    test_text_block(test);
    test_chunk_layout(test);
}
