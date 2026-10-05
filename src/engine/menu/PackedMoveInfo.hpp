#pragma once

#include <stdint.h>

namespace PackedMoveInfo {
constexpr uint8_t SLOT_COUNT = 4;
constexpr uint8_t BYTE_COUNT = 5;

inline void write(uint8_t *packed, uint8_t slot, uint16_t info) {
    if (slot >= SLOT_COUNT) return;
    const uint8_t byte = static_cast<uint8_t>(slot + (slot >> 2));
    const uint8_t shift = static_cast<uint8_t>((slot & 3) << 1);
    const uint16_t mask = static_cast<uint16_t>(0x03ffu << shift);
    uint16_t window = static_cast<uint16_t>(packed[byte]) |
                      (static_cast<uint16_t>(packed[byte + 1]) << 8);
    window = static_cast<uint16_t>((window & ~mask) | ((info << shift) & mask));
    packed[byte] = static_cast<uint8_t>(window);
    packed[byte + 1] = static_cast<uint8_t>(window >> 8);
}

inline uint16_t read(const uint8_t *packed, uint8_t slot) {
    if (packed == nullptr || slot >= SLOT_COUNT) return 0;
    const uint8_t byte = static_cast<uint8_t>(slot + (slot >> 2));
    const uint8_t shift = static_cast<uint8_t>((slot & 3) << 1);
    const uint16_t window = static_cast<uint16_t>(packed[byte]) |
                            (static_cast<uint16_t>(packed[byte + 1]) << 8);
    return static_cast<uint16_t>((window >> shift) & 0x03ffu);
}
} // namespace PackedMoveInfo
