#include "PlantStage.hpp"

void PlantStage::increment(uint8_t index) {
    const uint8_t byteIndex = index >> 2;
    const uint8_t shift = static_cast<uint8_t>((index & 0x03) << 1);
    const uint8_t stage = static_cast<uint8_t>(((value[byteIndex] >> shift) + 1) & 0x03);
    const uint8_t mask = static_cast<uint8_t>(0x03u << shift);
    value[byteIndex] = static_cast<uint8_t>((value[byteIndex] & ~mask) |
                                            (stage << shift));
}

void PlantStage::incrementAll() {
    // Per two-bit slot: 0->0 (empty never self-plants), 1->2, 2->3, 3->3
    // (mature holds instead of wrapping to empty). The even-bit/odd-bit masks
    // work every slot in a byte at once: the low bit of each pair shifts up and
    // the high bit is kept, so 3 stays 3 rather than carrying out.
    for (uint8_t i = 0; i < sizeof(value); ++i) {
        const uint8_t low = static_cast<uint8_t>(value[i] & 0x55u);
        const uint8_t high = static_cast<uint8_t>(value[i] & 0xAAu);
        const uint8_t carried = static_cast<uint8_t>((low << 1) | high);
        value[i] = static_cast<uint8_t>((carried & 0xAAu) | (high >> 1));
    }
}

uint8_t PlantStage::getStage(uint8_t index) {
    return static_cast<uint8_t>((value[index >> 2] >> ((index & 0x03) << 1)) & 0x03);
}