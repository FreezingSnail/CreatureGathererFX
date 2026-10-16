#pragma once
#include <stdint.h>
#include "../lib/uint24.h"
constexpr uint8_t STORE_RECORD_TOMBSTONE_ID = 0xff;
constexpr uint8_t STORE_RECORD_BYTES = 8;
constexpr uint8_t STORE_CAPACITY = 64;
constexpr uint16_t STORE_RECORDS_PER_SECTOR = STORE_CAPACITY;
struct __attribute__((packed)) StoreRecord {
    uint8_t id;
    uint16_t exp;
    // Folded 5-bit duel attack set.
    uint8_t attackMask;
    // Three spare wire bytes reclaimed from the retired moves[4]; always 0.
    uint8_t reserved[3];
    // 0 preserves the legacy/default-full interpretation; otherwise HP is value - 1.
    uint8_t hpEncoded;
};
static_assert(sizeof(StoreRecord) == 8, "Store record wire size");
// Callers enforce slot < STORE_CAPACITY before using this sole address helper.
inline uint24_t storeRecordAddr(uint24_t base, uint16_t slot) {
    return base + 112 + (static_cast<uint24_t>(slot) << 3);
}
inline void storeRecordEncode(const StoreRecord &in, uint8_t *out) {
    out[0] = in.id; out[1] = in.exp; out[2] = in.exp >> 8;
    out[3] = in.attackMask; out[4] = in.reserved[0]; out[5] = in.reserved[1]; out[6] = in.reserved[2];
    out[7] = in.id == STORE_RECORD_TOMBSTONE_ID ? 0 : in.hpEncoded;
}
inline void storeRecordDecode(const uint8_t *in, StoreRecord &out) {
    out.id = in[0]; out.exp = static_cast<uint16_t>(in[1]) | (static_cast<uint16_t>(in[2]) << 8);
    out.attackMask = in[3];
    out.reserved[0] = in[4]; out.reserved[1] = in[5]; out.reserved[2] = in[6];
    out.hpEncoded = in[7];
}
