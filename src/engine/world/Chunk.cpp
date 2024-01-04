#include "Chunk.hpp"
#include "../../common.hpp"
#include "../../fxdata/fxdata.h"

constexpr uint8_t CHUNK_SIZE = 4;

void Chunk::loadChunck(uint8_t m, uint8_t n, uint8_t chunk_index) {
    // Calculate the number of chunks per row
    int chunks_per_row = m / CHUNK_SIZE;

    // Calculate the starting index of the chunk
    int start_row = (chunk_index / chunks_per_row) * CHUNK_SIZE;
    int start_col = (chunk_index % chunks_per_row) * CHUNK_SIZE;

    // Generate the indices of the 4x4 chunk
    uint8_t index = 0;
    for (int i = 0; i < CHUNK_SIZE; ++i) {
        for (int j = 0; j < CHUNK_SIZE; j += 2) {
            uint8_t index1 = ((start_row + i) * m) + (start_col + j);
            uint8_t index2 = index1 + 1;

            uint8_t packed_indices = (FX::readIndexedUInt8(testmap, index1) << 4) | (FX::readIndexedUInt8(testmap, index2));
            tiles[index] = packed_indices;
            index++;
            // debug[i * CHUNK_SIZE + j] = FX::readIndexedUInt8(testmap, index1);
            // debug[i * CHUNK_SIZE + j + 1] = FX::readIndexedUInt8(testmap, index2);
            //  debug[i * CHUNK_SIZE + j] = index1;
            //  debug[i * CHUNK_SIZE + j + 1] = index2;
        }
    }
}
