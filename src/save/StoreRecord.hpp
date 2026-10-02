#pragma once

#include <stddef.h>
#include <stdint.h>

#include "../lib/uint24.h"

constexpr uint8_t STORE_RECORD_TOMBSTONE_ID = 0xff;
constexpr uint8_t STORE_RECORD_BYTES = 8;
constexpr uint16_t STORE_RECORDS_PER_SECTOR = 512;

// A store slot is exactly eight bytes, including the extension byte.
// A zero reserved byte means the default value for its eventual use.
struct __attribute__((packed)) StoreRecord {
    uint8_t id;
    uint16_t exp;
    uint8_t moves[4];
    uint8_t reserved;
};

static_assert(sizeof(StoreRecord) == STORE_RECORD_BYTES, "Store records must fit 32 per page");
static_assert(offsetof(StoreRecord, exp) == 1, "Experience starts after the id");
static_assert(offsetof(StoreRecord, moves) == 3, "Moves follow experience");
static_assert(offsetof(StoreRecord, reserved) == 7, "Reserved is the final byte");
static_assert(STORE_RECORD_BYTES * STORE_RECORDS_PER_SECTOR == 4096,
              "A store sector must hold 512 records");

inline uint24_t storeRecordAddr(uint24_t base, uint16_t slot) {
    return base + (static_cast<uint24_t>(slot) << 3);
}
