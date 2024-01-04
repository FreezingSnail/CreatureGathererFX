#pragma once
#include "Chunk.hpp"

enum Direction { Up, Right, Down, Left };
class ChunkMap {
  public:
    uint8_t currentChunkIndex;
    uint8_t chunkWidth, chunkHeight;
    Chunk *map[12];
    Chunk chunks[12];
    void loadChunkMap();
    void shiftChunks(Direction direction);
};