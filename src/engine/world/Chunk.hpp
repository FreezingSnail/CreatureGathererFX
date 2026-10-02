#pragma once

#include <stdint.h>

#include "../../lib/uint24.h"

namespace Chunk {

constexpr uint16_t MAP_WIDTH_TILES = 256;
constexpr uint16_t MAP_HEIGHT_TILES = 256;
constexpr uint8_t CHUNK_WIDTH_TILES = 8;
constexpr uint8_t CHUNK_HEIGHT_TILES = 4;
constexpr uint8_t CHUNKS_PER_ROW = 32;
constexpr uint8_t CHUNKS_PER_COLUMN = 64;
constexpr uint16_t CHUNK_COUNT = 2048;
constexpr uint8_t MAP_CHUNK_TILES = 32;
constexpr uint8_t MAP_CHUNK_BYTES = 64;
constexpr uint8_t SCRIPT_SLOT_BYTES = 128;
constexpr uint8_t SCRIPT_SLOT_SHIFT = 7;

inline uint16_t chunkAt(uint8_t x, uint8_t y) {
    return static_cast<uint16_t>(y / CHUNK_HEIGHT_TILES) * CHUNKS_PER_ROW +
           x / CHUNK_WIDTH_TILES;
}

inline uint16_t chunkOfLocation(uint16_t location) {
    return chunkAt(static_cast<uint8_t>(location % MAP_WIDTH_TILES),
                   static_cast<uint8_t>(location / MAP_WIDTH_TILES));
}

inline bool validChunkId(uint16_t chunkId) { return chunkId < CHUNK_COUNT; }

// Callers pass a valid chunk id. Widen before shifting: a 16-bit intermediate
// would wrap for the last script slots on AVR.
inline uint24_t mapChunkAddr(uint24_t mapBase, uint16_t chunkId) {
    return mapBase + static_cast<uint24_t>(chunkId) * MAP_CHUNK_BYTES;
}

inline uint24_t scriptSlotAddr(uint24_t scriptsBase, uint16_t chunkId) {
    return scriptsBase + (static_cast<uint24_t>(chunkId) << SCRIPT_SLOT_SHIFT);
}

inline bool chunkChanged(uint16_t previousChunk, uint16_t newChunk) {
    return previousChunk != newChunk;
}

} // namespace Chunk
