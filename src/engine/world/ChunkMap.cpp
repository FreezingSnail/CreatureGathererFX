#include "ChunkMap.hpp"

#define CHUNKS_WIDTH 4
#define CHUNKS_HEIGHT 3
#define CHUNK_OFFSET 16 * 4

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
    chunkM = m / 4;
    chunkN = n / 4;

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

/*  m = 3 m = 4

    0 1 2
    3 4 5
    6 7 8
    9 10 11

*/

void ChunkMap::shiftChunks(Direction direction) {
    Chunk *tmp[CHUNKS_WIDTH];
    switch (direction) {
    case Up:

        for (uint8_t i = 0; i < 4; i++) {
            tmp[i] = map[i];
        }
        for (uint8_t i = 0; i < 8; i++) {
            map[i] = map[i + CHUNKS_WIDTH];
            debug[i] = debug[i + CHUNKS_WIDTH];
        }
        for (uint8_t i = 8; i < 12; i++) {
            map[i] = tmp[i - 8];
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
        for (uint8_t i = 11; i >= CHUNKS_WIDTH; i--) {
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
    map[1]->drawChunk(0, (yTileOffset * 16) - (4 * 16), 0, ticker);
    map[2]->drawChunk(CHUNK_OFFSET, (yTileOffset * 16) - (4 * 16), 0, ticker);
    map[3]->drawChunk(CHUNK_OFFSET * 2, (yTileOffset * 16) - (4 * 16), 0, ticker);

    map[5]->drawChunk(0, (yTileOffset * 16), 0, ticker);
    map[6]->drawChunk(CHUNK_OFFSET, (yTileOffset * 16), 0, ticker);
    map[7]->drawChunk(CHUNK_OFFSET * 2, (yTileOffset * 16), 0, ticker);

    map[9]->drawChunk(0, (yTileOffset * 16) + (4 * 16), 0, ticker);
    map[10]->drawChunk(CHUNK_OFFSET, (yTileOffset * 16) + (4 * 16), 0, ticker);
    map[11]->drawChunk(CHUNK_OFFSET * 2, (yTileOffset * 16) + (4 * 16), 0, ticker);
}

void ChunkMap::indexToChunkCord(uint8_t index, uint8_t *x, uint8_t *y) {
}