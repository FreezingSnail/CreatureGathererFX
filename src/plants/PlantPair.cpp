#include "PlantPair.hpp"

bool PlantPair::isActive(uint8_t index) {
    return (value >> (index * 4)) >> 3 & 1;
}

PlantType PlantPair::type(uint8_t index) {
    return (PlantType)((value >> (index * 4)) & 0b111);
}

// 0000 0000
void PlantPair::setActive(uint8_t index, bool active) {
    if (active) {
        value |= (1 << (index * 4) << 3);
    } else {
        value &= ~(1 << (index * 4) << 3);
    }
}

void PlantPair::setType(uint8_t index, PlantType type) {
    // Clear the bits at the target position in num
    uint8_t mask = ~(0b111 << (index * 4));
    value &= mask;

    // Write the incremented bits to the target position in num
    value |= (type << (index * 4));
}