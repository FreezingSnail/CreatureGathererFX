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
    uint8_t moves[4];
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
    for (uint8_t i = 0; i < 4; ++i) out[3+i] = in.moves[i];
    out[7] = in.id == STORE_RECORD_TOMBSTONE_ID ? 0 : in.hpEncoded;
}
inline void storeRecordDecode(const uint8_t *in, StoreRecord &out) {
    out.id = in[0]; out.exp = static_cast<uint16_t>(in[1]) | (static_cast<uint16_t>(in[2]) << 8);
    for (uint8_t i = 0; i < 4; ++i) out.moves[i] = in[3+i];
    out.hpEncoded = in[7];
}
