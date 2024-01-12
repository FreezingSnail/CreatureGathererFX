#pragma once
#include "Chunk.hpp"

enum Direction { Up, Right, Down, Left };
class ChunkMap {
  public:
    uint8_t currentChunkIndex;
    uint8_t chunkWidth, chunkHeight, m, n;
    Chunk *map[12];
    Chunk chunks[12];
    uint8_t debug[12];
    void loadChunkMap();
    void shiftChunks(Direction direction);

    void indexToChunkCord(uint8_t index, uint8_t *x, uint8_t *y);

    void drawChunkMap(uint8_t curX, uint8_t curY);
};