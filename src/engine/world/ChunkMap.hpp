#pragma once
#include "Chunk.hpp"

enum Direction { Up, Right, Down, Left };
class ChunkMap {
  public:
    uint8_t currentChunkIndex;
    uint8_t chunkWidth, chunkHeight, m, n, chunkM, chunkN;
    Chunk *map[9];
    Chunk chunks[9];
    uint8_t debug[9];
    void loadChunkMap();
    void shiftChunks(Direction direction);

    void indexToChunkCord(uint8_t index, uint8_t *x, uint8_t *y);

    void drawChunkMap(uint8_t ticker, uint8_t xTileOffset, uint8_t yTileOffset);
};