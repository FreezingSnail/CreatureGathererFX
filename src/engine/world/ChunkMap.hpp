#pragma once
#include "Chunk.hpp"

class ChunkMap {
  public:
    Chunk *chunks[9];
    void loadChunkMap(uint8_t mapIndex, uint8_t submapIndex);
};