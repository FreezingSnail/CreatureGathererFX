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
    uint8_t tiles[16];
    uint16_t debug[16];
    // uint8_t offset[16];
    // uint8_t value[16];
    uint8_t chunkIndex;
    void loadChunck(uint8_t m, uint8_t n, uint8_t index);
    void drawChunk(int8_t x, int8_t y, int8_t xOffset, int8_t yOffset);
};
