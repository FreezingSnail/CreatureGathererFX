#include "PlantStage.hpp"

void PlantStage::increment(uint8_t index) {
    // Shift the index to account for 2-bit pairs
    index *= 2;

    // Extract the 2-bit pair at the target position
    uint64_t bits = (value >> index) & 0b11;

    // Increment the 2-bit pair
    bits = (bits + 1) & 0b11;

    // Clear the bits at the target position in num
    uint64_t mask = ~(0b11ULL << index);
    value &= mask;

    // Write the incremented bits to the target position in num
    value |= (bits << index);
}

void PlantStage::incrementAll() {
    for (uint8_t i = 0; i < 32; i++) {
        increment(i);
    }
}

uint8_t PlantStage::read(uint8_t index) {
    return value >> (index * 2) & 0b11;
}