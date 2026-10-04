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
    for (uint8_t i = 0; i < 32; i++) {
        increment(i);
    }
}

uint8_t PlantStage::getStage(uint8_t index) {
    return static_cast<uint8_t>((value[index >> 2] >> ((index & 0x03) << 1)) & 0x03);
}