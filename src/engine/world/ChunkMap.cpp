#include "ChunkMap.hpp"

#define CHUNKS_WIDTH 3
#define CHUNKS_HEIGHT 3
#define CHUNK_OFFSET 16 * 4

/**
    * Given the chunk map:
    * 0  1  2
    * 3  4  5
    * 6  7  8
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
    // load 0-3,
    // load m + 0-3
    // load 2m + 0-3

    currentChunkIndex = 5;
    m = 24;
    n = 24;
    chunkM = m / 4;
    chunkN = n / 4;

    for (int i = 0; i < 3; ++i) {
        for (uint8_t j = 0; j < 3; j++) {
            uint8_t chunk_index = (chunkM * i) + j;
            uint8_t index = j + (i * 3);
            chunks[index].loadChunck(m, n, chunk_index);
            map[index] = &chunks[index];
            debug[index] = chunk_index;
        }
    }
}

/*  m = 3 m = 4

    0 1 2
    3 4 5
    6 7 8

*/

void ChunkMap::shiftChunks(Direction direction) {
    Chunk *tmp[CHUNKS_WIDTH];
    switch (direction) {
    case Up:

        for (uint8_t i = 0; i < 3; i++) {
            tmp[i] = map[i];
        }
        for (uint8_t i = 0; i < 5; i++) {
            map[i] = map[i + CHUNKS_WIDTH];
            debug[i] = debug[i + CHUNKS_WIDTH];
        }
        for (uint8_t i = 6; i < 9; i++) {
            map[i] = tmp[i - 6];
            debug[i] = currentChunkIndex - 1 + i + chunkM;
            map[i]->loadChunck(24, 24, currentChunkIndex - 1 + i + chunkM);
        }
        currentChunkIndex += chunkM;
        break;
    case Down:
        // Store the last row in temporary storage
        for (uint8_t i = 0; i < CHUNKS_WIDTH; i++) {
            tmp[i] = map[8 + i];
        }

        // Shift the first two rows down
        for (uint8_t i = 8; i >= CHUNKS_WIDTH; i--) {
            map[i] = map[i - CHUNKS_WIDTH];
            debug[i] = debug[i - CHUNKS_WIDTH];
        }

        // Restore the last row from temporary storage
        for (uint8_t i = 0; i < CHUNKS_WIDTH; i++) {
            map[i] = tmp[i];
            debug[i] = currentChunkIndex + 1 + i - chunkM;
            map[i]->loadChunck(24, 24, currentChunkIndex + 1 + i - chunkM);
        }
        currentChunkIndex -= chunkM;

        break;

    case Right:
        currentChunkIndex++;
        tmp[0] = map[2];
        tmp[1] = map[5];
        tmp[2] = map[8];
        for (int i = 3; i > 0; i++) {
            map[i] = map[i + 1];
        }
        for (int i = 3; i > 0; i++) {
            map[i + chunkWidth] = map[i + chunkWidth + 1];
        }
        for (int i = 3; i > 0; i++) {
            map[i + (2 * chunkWidth)] = map[i + (2 * chunkWidth) + 1];
        }

        map[2] = tmp[0];
        map[5] = tmp[1];
        map[8] = tmp[2];
        map[2]->loadChunck(chunkWidth, chunkHeight, currentChunkIndex + 2 - chunkWidth);
        map[5]->loadChunck(chunkWidth, chunkHeight, currentChunkIndex + 2);
        map[8]->loadChunck(chunkWidth, chunkHeight, currentChunkIndex + 2 + chunkWidth);
        break;

    case Left:
        currentChunkIndex--;
        tmp[0] = map[0];
        tmp[1] = map[3];
        tmp[2] = map[6];

        for (int i = 0; i < 3; i++) {
            map[i + 1] = map[i];
        }
        for (int i = 0; i < 3; i++) {
            map[i + chunkWidth + 1] = map[i + chunkWidth];
        }
        for (int i = 0; i < 3; i++) {
            map[i + (2 * chunkWidth) + 1] = map[i + (2 * chunkWidth)];
        }

        map[2] = tmp[0];
        map[5] = tmp[1];
        map[6] = tmp[2];
        map[2]->loadChunck(chunkWidth, chunkHeight, currentChunkIndex - 1 - chunkWidth);
        map[5]->loadChunck(chunkWidth, chunkHeight, currentChunkIndex - 1);
        map[6]->loadChunck(chunkWidth, chunkHeight, currentChunkIndex - 1 + chunkWidth);
        break;
    }
}

void ChunkMap::drawChunkMap(uint8_t ticker, uint8_t xTileOffset, uint8_t yTileOffset) {
    // viewport of 128x64 pixels or 8x4 tiles at any given time
    // need to overdraw 1 tile in each direction incase of movement,
    // so 10x6 tiles need to be drawn
    // always start at center left (map index 5) chunk
    // for (uint8_t i = 0; i < 10; i++) {
    //     for (uint8_t j = 0; j < 6; j++) {
    //     }
    // }

    // for (uint8_t i = 0; i < 3; i++) {
    //     for (uint8_t j = 0; j < 4; j++) {
    //         uint8_t y = (i * 4 * 16) - 16;
    //         uint8_t x = (j * 4 * 16) - 16;
    //         map[j + (4 * i)]->drawChunk(x, y, mapX, mapY);
    //     }
    // }

    map[0]->drawChunk(0, (yTileOffset * 16) - (4 * 16), 0, ticker);
    map[1]->drawChunk(CHUNK_OFFSET, (yTileOffset * 16) - (4 * 16), 0, ticker);
    map[2]->drawChunk(CHUNK_OFFSET * 2, (yTileOffset * 16) - (4 * 16), 0, ticker);

    map[3]->drawChunk(0, (yTileOffset * 16), 0, ticker);
    map[4]->drawChunk(CHUNK_OFFSET, (yTileOffset * 16), 0, ticker);
    map[5]->drawChunk(CHUNK_OFFSET * 2, (yTileOffset * 16), 0, ticker);

    map[6]->drawChunk(0, (yTileOffset * 16) + (4 * 16), 0, ticker);
    map[7]->drawChunk(CHUNK_OFFSET, (yTileOffset * 16) + (4 * 16), 0, ticker);
    map[8]->drawChunk(CHUNK_OFFSET * 2, (yTileOffset * 16) + (4 * 16), 0, ticker);
}

void ChunkMap::indexToChunkCord(uint8_t index, uint8_t *x, uint8_t *y) {
}