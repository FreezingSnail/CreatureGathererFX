#pragma once

#include "test.hpp"
#include "../src/engine/world/Chunk.hpp"
#include "../src/vm/ScriptVM.hpp"
#include "../fxdata/generated/scripts.hpp"

void ChunkTest(TestSuite &suite) {
    Test test = Test(__func__);
    test.assert(Chunk::MAP_WIDTH_TILES, static_cast<uint16_t>(256), "map width");
    test.assert(Chunk::MAP_HEIGHT_TILES, static_cast<uint16_t>(256), "map height");
    test.assert(Chunk::MAP_CHUNK_TILES, static_cast<uint8_t>(32), "map chunk tile count");
    test.assert(Chunk::MAP_CHUNK_BYTES, static_cast<uint8_t>(64), "map chunk byte count");
    test.assert(Chunk::SCRIPT_SLOT_BYTES, static_cast<uint8_t>(128), "script slot byte count");
    test.assert(Chunk::CHUNKS_PER_ROW, static_cast<uint8_t>(32), "chunks per row");
    test.assert(Chunk::CHUNKS_PER_COLUMN, static_cast<uint8_t>(64), "chunks per column");
    test.assert(Chunk::CHUNK_COUNT, static_cast<uint16_t>(2048), "chunk count");

    test.assert(Chunk::chunkAt(7, 3), static_cast<uint16_t>(0), "last tile in first chunk");
    test.assert(Chunk::chunkAt(8, 3), static_cast<uint16_t>(1), "x boundary");
    test.assert(Chunk::chunkAt(7, 4), static_cast<uint16_t>(32), "y boundary");
    test.assert(Chunk::chunkAt(8, 4), static_cast<uint16_t>(33), "diagonal boundary");
    test.assert(Chunk::chunkAt(248, 252), static_cast<uint16_t>(2047), "last chunk start");
    test.assert(Chunk::chunkAt(255, 255), static_cast<uint16_t>(2047), "last map tile");
    test.assert(Chunk::chunkOfLocation(To1D(255, 255)),
                static_cast<uint16_t>(2047), "last flat location");
    test.assert(Chunk::validChunkId(2047), true, "final chunk valid");
    test.assert(Chunk::validChunkId(2048), false, "next chunk invalid");

    const uint24_t base = 0x12000;
    test.assert(Chunk::mapChunkAddr(base, 3), static_cast<uint24_t>(base + 192),
                "map chunk 3 uses 64 bytes");
    test.assert(Chunk::mapChunkAddr(base, 2047), static_cast<uint24_t>(base + 131008),
                "last map chunk address");
    test.assert(Chunk::scriptSlotAddr(base, 3), static_cast<uint24_t>(base + 384),
                "script slot 3 uses 128 bytes");
    test.assert(Chunk::scriptSlotAddr(base, 2047), static_cast<uint24_t>(base + 262016),
                "last script slot address does not wrap");
    test.assert(Chunk::scriptSlotAddr(base, 0), base, "blob0 slot");
    test.assert(Chunk::scriptSlotAddr(base, 32), static_cast<uint24_t>(base + 4096),
                "blob32 slot");
    test.assert(Chunk::scriptSlotAddr(base, 33), static_cast<uint24_t>(base + 4224),
                "blob33 slot");
    test.assert(sizeof(blob0), static_cast<size_t>(15), "generated blob0 prefix size");
    test.assert(sizeof(blob32), static_cast<size_t>(9), "generated blob32 prefix size");
    test.assert(sizeof(blob33), static_cast<size_t>(9), "generated blob33 prefix size");
    test.assert(blob0[0], static_cast<uint8_t>(5), "generated blob0 first opcode");
    test.assert(blob32[0], static_cast<uint8_t>(4), "generated blob32 first opcode");
    test.assert(blob33[0], static_cast<uint8_t>(4), "generated blob33 first opcode");

    test.assert(Chunk::chunkChanged(Chunk::chunkAt(7, 3), Chunk::chunkAt(7, 3)), false,
                "no transition within a chunk");
    test.assert(Chunk::chunkChanged(Chunk::chunkAt(7, 3), Chunk::chunkAt(8, 3)), true,
                "transition at x 8");
    test.assert(Chunk::chunkChanged(Chunk::chunkAt(7, 3), Chunk::chunkAt(7, 4)), true,
                "transition at y 4");
    suite.addTest(test);
}

void ChunkSuite(TestRunner &runner) {
    TestSuite suite = TestSuite("Chunk Suite");
    ChunkTest(suite);
    runner.addTestSuite(suite);
}
