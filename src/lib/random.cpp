#include "random.hpp"

namespace {
uint16_t state = 1;
}

void rngSeed(uint16_t seed) {
    state = seed == 0 ? 1 : seed;
}

uint8_t rngNext8() {
    uint16_t value = state;
    value ^= static_cast<uint16_t>(value << 7);
    value ^= static_cast<uint16_t>(value >> 9);
    value ^= static_cast<uint16_t>(value << 8);
    state = value;
    return static_cast<uint8_t>(value >> 8);
}

bool randomRoll(uint8_t l, uint8_t r, uint8_t target) {
    return randomRoll(l, r) == target;
}

uint8_t randomRoll(uint8_t l, uint8_t r) {
    const uint16_t span = static_cast<uint16_t>(r) - l + 1;
    const uint8_t next = rngNext8();
    return static_cast<uint8_t>(l +
        (static_cast<uint16_t>(next) * span >> 8));
}
