#include "Chunk.hpp"
#include "../../common.hpp"
#include "../../fxdata/fxdata.h"

/*
    128 x 64 pixels -> 16x16 tiles
    8 tiles across and 4 tiles tall
    2 4x4 tiles
    16 tiles per chunk
*/

#define TILE_SIZE 16
constexpr uint8_t CHUNK_SIZE = 4;

void Chunk::loadChunck(uint8_t m, uint8_t n, uint8_t chunk_index) {
    // Calculate the number of chunks per row
    uint8_t chunks_per_row = m / CHUNK_SIZE;

    // Calculate the starting index of the chunk
    uint8_t start_row = (chunk_index / chunks_per_row) * CHUNK_SIZE;
    uint8_t start_col = (chunk_index % chunks_per_row) * CHUNK_SIZE;

    // Generate the indices of the 4x4 chunk
    uint8_t index = 0;
    for (uint8_t i = 0; i < CHUNK_SIZE; i++) {
        for (uint8_t j = 0; j < CHUNK_SIZE; j++) {
            uint8_t dataIndex = ((start_row + i) * m) + (start_col + j);

            tiles[index] = FX::readIndexedUInt8(testmap, dataIndex);
            // debug[index] = dataIndex;
            index++;
        }
    }
}

void Chunk::drawChunk(int8_t x, int8_t y, int8_t xOffset, int8_t yOffset) {
    for (uint8_t i = 0; i < CHUNK_SIZE; i++) {
        for (uint8_t j = 0; j < CHUNK_SIZE; j++) {
            uint8_t index = (i * CHUNK_SIZE) + j;
            uint8_t tile = tiles[index];
            int8_t tilex = x + (j * TILE_SIZE) + xOffset - 11;
            int8_t tiley = y + (i * TILE_SIZE) + yOffset - 8;
            FX::drawBitmap(tilex, tiley, worldTiles, tile, dbmNormal);
        }
    }
}
