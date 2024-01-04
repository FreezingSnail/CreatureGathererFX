#include "ChunkMap.hpp"

#define CHUNKS_WIDTH 4
#define CHUNKS_HEIGHT 3

/**
    * Given the chunk map:
    * 0  1  2  3
    * 4  5  6  7
    * 8  9  10 11
    *
    * The current chunk index is 5
    * as the player is in the center of the screen
    * the center chunk will be the one to the center left

*/

void ChunkMap::loadChunkMap() {
    currentChunkIndex = 5;
    uint8_t m = 18;
    uint8_t n = 18;
    uint8_t chunk_index = 0;
    for (int i = 0; i < 9; ++i) {
        map[i]->loadChunck(m, n, chunk_index);
        chunk_index++;
    }
}

void ChunkMap::shiftChunks(Direction direction) {
    switch (direction) {
    case Up:
        currentChunkIndex + 3;
        for (int i = 0; i < 6; i++) {
            map[i] = map[i + CHUNKS_WIDTH];
        }
        for (uint8_t i = 8; i < 12; i++) {
            map[i]->loadChunck(chunkWidth, chunkHeight, currentChunkIndex - 3 + i);
        }
        break;
    case Down:
        currentChunkIndex - 3;
        for (int i = 11; i > 3; i--) {
            map[i] = map[i - CHUNKS_WIDTH];
        }
        for (uint8_t i = 0; i < 3; i++) {
            map[i]->loadChunck(chunkWidth, chunkHeight, currentChunkIndex - 3 + i);
        }
        break;

    case Right:
        currentChunkIndex++;
        for (int i = 3; i > 0; i++) {
            map[i] = map[i + 1];
        }
        for (int i = 3; i > 0; i++) {
            map[i + chunkWidth] = map[i + chunkWidth + 1];
        }
        for (int i = 3; i > 0; i++) {
            map[i + (2 * chunkWidth)] = map[i + (2 * chunkWidth) + 1];
        }

        map[3]->loadChunck(chunkWidth, chunkHeight, currentChunkIndex + 2 - chunkWidth);
        map[7]->loadChunck(chunkWidth, chunkHeight, currentChunkIndex + 2);
        map[11]->loadChunck(chunkWidth, chunkHeight, currentChunkIndex + 2 + chunkWidth);
        break;

    case Left:
        currentChunkIndex--;
        for (int i = 0; i < 3; i++) {
            map[i + 1] = map[i];
        }
        for (int i = 0; i < 3; i++) {
            map[i + chunkWidth + 1] = map[i + chunkWidth];
        }
        for (int i = 0; i < 3; i++) {
            map[i + (2 * chunkWidth) + 1] = map[i + (2 * chunkWidth)];
        }

        map[0]->loadChunck(chunkWidth, chunkHeight, currentChunkIndex - 1 - chunkWidth);
        map[4]->loadChunck(chunkWidth, chunkHeight, currentChunkIndex - 1);
        map[8]->loadChunck(chunkWidth, chunkHeight, currentChunkIndex - 1 + chunkWidth);
        break;
    }
}