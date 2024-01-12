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

    128 x 64 pixels -> 16x16 tiles
    8 tiles across and 4 tiles tall
    2 4x4 tiles
    16 tiles per chunk

*/

void ChunkMap::loadChunkMap() {
    // need to load 3 rows of 4 chunks
    // given mxn map, the total chunks are
    // m/4 * n/4 or ChunkM x chunkN
    // assume start on chunk 0, top left
    // load 0-4,
    // load m + 0-4
    // load 2m + 0-4

    currentChunkIndex = 5;
    m = 24;
    n = 24;
    uint8_t chunkM = m / 4;
    uint8_t chunkN = n / 4;

    for (int i = 0; i < 3; ++i) {
        for (uint8_t j = 0; j < 4; j++) {
            uint8_t chunk_index = (chunkM * i) + j;
            uint8_t index = j + (i * 4);
            chunks[index].loadChunck(m, n, chunk_index);
            map[index] = &chunks[index];
            debug[index] = chunk_index;
        }
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

void ChunkMap::drawChunkMap(uint8_t curX, uint8_t curY) {
    // viewport of 128x64 pixels or 8x4 tiles at any given time
    // need to overdraw 1 tile in each direction incase of movement,
    // so 10x6 tiles need to be drawn
    // always start at center left (map index 5) chunk
    // for (uint8_t i = 0; i < 10; i++) {
    //     for (uint8_t j = 0; j < 6; j++) {
    //     }
    // }

    for (uint8_t i = 0; i < 3; i++) {
        for (uint8_t j = 0; j < 4; j++) {
            uint8_t y = (i * 4 * 16) - 16;
            uint8_t x = (j * 4 * 16) - 16;
            map[j + (4 * i)]->drawChunk(x, y, curX, curY);
        }
    }
}

void ChunkMap::indexToChunkCord(uint8_t index, uint8_t *x, uint8_t *y) {
}