#pragma once
#include <stdint.h>

constexpr uint8_t frontNibble(uint8_t byte) {
    return byte >> 4;
}

constexpr uint8_t backNibble(uint8_t byte) {
    return byte & 0x0F;
}

class Chunk {
  public:
    uint8_t tiles[8];
    // uint8_t debug[16];
    void loadChunck(uint8_t m, uint8_t n, uint8_t index);
};
