#pragma once

#include <string.h>

#include "fxtest.hpp"
#include "generated/map_data.hpp"
#include "generated/script_data.hpp"
#include "src/engine/world/Chunk.hpp"
#include "src/GameState.hpp"
#include "src/engine/world/TilePropertyWindow.hpp"
#include "src/engine/world/World.hpp"
#include "src/engine/world/WorldCollision.hpp"
#include "src/fxdata.h"
#include "src/lib/ReadData.hpp"
#include "src/lib/FxRead.hpp"
#include "src/lib/FxReadCounter.hpp"

extern GameState gameState;

namespace {

uint16_t rawTileWordAt(uint8_t x, uint8_t y) {
    const uint32_t offset =
        (static_cast<uint32_t>(y) * 256u + x) * sizeof(uint16_t);
    const uint24_t address = raw_map_data + static_cast<uint24_t>(offset);
    return ReadFXu16(address);
}

uint16_t rawTileWordAtIndex(uint16_t index) {
    const uint24_t address = raw_map_data + static_cast<uint24_t>(index) * 2u;
    return ReadFXu16(address);
}

// Packed-word walkability exactly as WorldCollision::canEnter derives it: an
// empty GID is blocked even when the walkable property bit is present.
bool packedWalkableAt(int16_t x, int16_t y) {
    if (x < 0 || y < 0 || x >= static_cast<int16_t>(Chunk::MAP_WIDTH_TILES) ||
        y >= static_cast<int16_t>(Chunk::MAP_HEIGHT_TILES))
        return false;
    const uint16_t word = rawTileWordAt(static_cast<uint8_t>(x), static_cast<uint8_t>(y));
    return TileProps::tileId(word) != 0 &&
           TileProps::hasTileProp(word, TileProps::PROP_WALKABLE);
}

// Breadth-first search over the device's packed collision, collecting visited
// locations (y<<8|x) into a caller buffer. The authored pocket is sealed, so
// the frontier stays tiny; `cap` is a guard, not an expected size.
uint8_t clearingBfs(uint16_t start, uint16_t *seen, uint8_t cap) {
    uint8_t head = 0;
    uint8_t tail = 0;
    seen[tail++] = start;
    while (head < tail) {
        const int16_t x = static_cast<uint8_t>(seen[head] & 0xFF);
        const int16_t y = static_cast<uint8_t>(seen[head] >> 8);
        ++head;
        const int16_t candidates[4][2] = {
            {static_cast<int16_t>(x - 1), y}, {static_cast<int16_t>(x + 1), y},
            {x, static_cast<int16_t>(y - 1)}, {x, static_cast<int16_t>(y + 1)},
        };
        for (uint8_t i = 0; i < 4; ++i) {
            const int16_t nx = candidates[i][0];
            const int16_t ny = candidates[i][1];
            if (!packedWalkableAt(nx, ny)) continue;
            const uint16_t nloc = static_cast<uint16_t>(nx) |
                                  static_cast<uint16_t>(static_cast<uint16_t>(ny) << 8);
            bool known = false;
            for (uint8_t at = 0; at < tail; ++at)
                if (seen[at] == nloc) { known = true; break; }
            if (known) continue;
            if (tail >= cap) return tail;
            seen[tail++] = nloc;
        }
    }
    return tail;
}

void expectAddressBytes(FxTest &test, uint24_t actual, uint24_t expected,
                        const __FlashStringHelper *label) {
    uint8_t actualBytes[3];
    uint8_t expectedBytes[3];
    memcpy(actualBytes, &actual, sizeof(actualBytes));
    memcpy(expectedBytes, &expected, sizeof(expectedBytes));
    for (uint8_t byte = 0; byte < 3; ++byte) {
        test.expectEq(actualBytes[byte], expectedBytes[byte], label);
    }
}

} // namespace

inline void test_map_words(FxTest &test) {
    bool sawHighId = false;
    bool sawWalkable = false;
    bool sawWater = false;
    bool sawEncounter = false;
    for (uint8_t index = 0; index < mapWordFixtureCount; ++index) {
        const uint16_t cell = pgm_read_word(&mapWordFixtures[index].index);
        const uint16_t expected = pgm_read_word(&mapWordFixtures[index].word);
        const uint16_t actual = rawTileWordAtIndex(cell);
        test.expectEq(actual, expected, F("model raw map word"));
        test.expectEq(TileProps::tileId(actual), expected & TileProps::TILE_ID_MASK,
                      F("decoded map tile id"));
        test.expectEq(TileProps::tileProps(actual),
                      (expected & TileProps::TILE_PROP_MASK) >> TileProps::TILE_PROP_SHIFT,
                      F("decoded map properties"));
        sawHighId = sawHighId || TileProps::tileId(actual) > 511;
        sawWalkable = sawWalkable || TileProps::hasTileProp(actual, TileProps::PROP_WALKABLE);
        sawWater = sawWater || TileProps::hasTileProp(actual, TileProps::PROP_WATER);
        sawEncounter = sawEncounter || TileProps::hasTileProp(actual, TileProps::PROP_ENCOUNTER);
    }
    test.expectEq(sawHighId, true, F("fixture includes tile id above 511"));
    test.expectEq(sawWalkable, true, F("fixture includes walkable property"));
    test.expectEq(sawWater, true, F("fixture includes water property"));
    test.expectEq(sawEncounter, true, F("fixture includes encounter property"));
}

inline void test_chunk_slots(FxTest &test) {
    constexpr uint16_t chunks[] = {0, 3, 512, 2047};
    constexpr uint32_t mapOffsets[] = {0, 192, 32768, 131008};
    for (uint8_t index = 0; index < 4; ++index) {
        const uint16_t chunk = chunks[index];
        const uint24_t expectedMap = map_data + static_cast<uint24_t>(mapOffsets[index]);
        expectAddressBytes(test, Chunk::mapChunkAddr(map_data, chunk), expectedMap,
                           F("map chunk address bytes"));
    }

    bool sawChunk0 = false;
    bool sawChunk33 = false;
    bool sawChunk65 = false;
    for (uint16_t index = 0; index < scriptSlotFixtureCount; ++index) {
        const uint16_t chunk = pgm_read_word(&scriptSlotFixtures[index].chunk);
        sawChunk0 = sawChunk0 || chunk == 0;
        sawChunk33 = sawChunk33 || chunk == 33;
        sawChunk65 = sawChunk65 || chunk == 65;
        const uint24_t address = Chunk::scriptSlotAddr(scripts, chunk);
        uint8_t prefix[4];
        FxRead::bytes(address, prefix, sizeof(prefix));
        for (uint8_t byte = 0; byte < sizeof(prefix); ++byte) {
            test.expectEq(prefix[byte], pgm_read_byte(&scriptSlotFixtures[index].prefix[byte]),
                          F("real script prefix"));
        }
    }
    test.expectEq(sawChunk0, true, F("real script blob chunk 0"));
    test.expectEq(sawChunk33, true, F("real script blob chunk 33"));
    test.expectEq(sawChunk65, true, F("real script blob chunk 65"));

    for (uint8_t index = 0; index < scriptAddressFixtureCount; ++index) {
        const uint16_t chunk = pgm_read_word(&scriptAddressFixtures[index].chunk);
        const uint8_t low = pgm_read_byte(&scriptAddressFixtures[index].offset[0]);
        const uint8_t middle = pgm_read_byte(&scriptAddressFixtures[index].offset[1]);
        const uint8_t high = pgm_read_byte(&scriptAddressFixtures[index].offset[2]);
        const uint32_t offset = static_cast<uint32_t>(low) |
                                (static_cast<uint32_t>(middle) << 8) |
                                (static_cast<uint32_t>(high) << 16);
        const uint24_t expected = scripts + static_cast<uint24_t>(offset);
        expectAddressBytes(test, Chunk::scriptSlotAddr(scripts, chunk), expected,
                           F("synthetic script slot address bytes"));
        test.expectEq(offset, static_cast<uint32_t>(chunk) * 128u,
                      F("script slot offset math"));
        test.expectEq(offset + 128u <= 262144u, true,
                      F("script slot remains in script region"));
    }
}

inline void test_read_budget(FxTest &test) {
    gameState.state = GameState_t::WORLD;
    gameState.playerLocation = static_cast<uint16_t>((2u << 8) | 3u);
    WorldTransient &world = worldState();
    WorldEngine::init(world);
    world.motion.directionAndFlags = static_cast<uint8_t>(0x04u |
        static_cast<uint8_t>(Direction::RIGHT));
    world.motion.step = 0;

    FxReadCounter::resetFrame();
    for (uint8_t frame = 0; frame < 16; ++frame) WorldEngine::moveChar(world);
    test.expectEq(gameState.playerLocation,
                  static_cast<uint16_t>((2u << 8) | 4u),
                  F("completed step commits destination tile"));
    test.expectEq(FxReadCounter::exact(0), true,
                  F("completed step dispatch uses zero FX reads"));
    test.expectEq(FxReadCounter::markUpdate(), true,
                  F("step update passes exact FX budget"));
    test.expectEq(FxReadCounter::updateExactZero(), true,
                  F("step update has no additional FX reads"));
    test.expectEq(FxReadCounter::renderExact(0), true,
                  F("step frame render has no FX reads"));
    test.expectEq(FxReadCounter::framePassed(), true,
                  F("step frame satisfies FX read contract"));
}

// The map includes high-ID property fixtures on the sealed south border; the
// ordinary collision samples below remain on the reachable starting path.
inline void test_spawn_reachability(FxTest &test) {
    // First-beta clearing (data/beta-area.toml): the ordinary new-game start is
    // (11,5) and the authored pocket must let the player walk to the lure box
    // (9,2) and plant (14,2) approaches and back. Read the packed words from the
    // FX image so this pins the device bytes, not just the host source file.
    const uint16_t spawn = rawTileWordAt(11, 5);
    test.expectEq(TileProps::tileId(spawn), 275, F("beta spawn raw tile id"));
    test.expectEq(TileProps::hasTileProp(spawn, TileProps::PROP_WALKABLE), true,
                  F("beta spawn walkable property"));
    test.expectEq(TileProps::hasTileProp(spawn, TileProps::PROP_ENCOUNTER), true,
                  F("beta spawn encounter property"));

    // Every orthogonal neighbour of the start is enterable.
    test.expectEq(packedWalkableAt(10, 5), true, F("spawn west neighbour walkable"));
    test.expectEq(packedWalkableAt(12, 5), true, F("spawn east neighbour walkable"));
    test.expectEq(packedWalkableAt(11, 4), true, F("spawn north neighbour walkable"));
    test.expectEq(packedWalkableAt(11, 6), true, F("spawn south neighbour walkable"));

    // The lure box and plant stay non-walkable props faced from a walkable cell.
    const uint16_t box = rawTileWordAt(9, 2);
    test.expectEq(TileProps::tileId(box), 545, F("lure box raw tile id"));
    test.expectEq(TileProps::hasTileProp(box, TileProps::PROP_SPARE), true,
                  F("lure box carries spare prop"));
    test.expectEq(TileProps::hasTileProp(box, TileProps::PROP_WALKABLE), false,
                  F("lure box is not walkable"));
    test.expectEq(packedWalkableAt(10, 2), true, F("lure box approach walkable"));

    const uint16_t plant = rawTileWordAt(14, 2);
    test.expectEq(TileProps::tileId(plant), 551, F("plant raw tile id"));
    test.expectEq(TileProps::hasTileProp(plant, TileProps::PROP_PLANTABLE), true,
                  F("plant carries plantable prop"));
    test.expectEq(packedWalkableAt(13, 2), true, F("plant approach walkable"));

    // Sealed clearing: the start reaches exactly the 39 authored walkable cells,
    // including both interaction approaches; the route is reversible.
    uint16_t seen[64];
    const uint8_t fromStart = clearingBfs(static_cast<uint16_t>((5u << 8) | 11u), seen, 64);
    test.expectEq(fromStart, 39, F("beta clearing walkable cell count"));
    bool boxReachable = false;
    bool plantReachable = false;
    for (uint8_t at = 0; at < fromStart; ++at) {
        if (seen[at] == static_cast<uint16_t>((2u << 8) | 10u)) boxReachable = true;
        if (seen[at] == static_cast<uint16_t>((2u << 8) | 13u)) plantReachable = true;
    }
    test.expectEq(boxReachable, true, F("lure box approach reachable from spawn"));
    test.expectEq(plantReachable, true, F("plant approach reachable from spawn"));

    const uint8_t fromBox = clearingBfs(static_cast<uint16_t>((2u << 8) | 10u), seen, 64);
    bool startReachable = false;
    for (uint8_t at = 0; at < fromBox; ++at)
        if (seen[at] == static_cast<uint16_t>((5u << 8) | 11u)) startReachable = true;
    test.expectEq(fromBox, 39, F("return traversal walkable cell count"));
    test.expectEq(startReachable, true, F("return path reaches the spawn"));
}

// The map includes high-ID property fixtures on the sealed south border; the
// ordinary collision samples below remain on the reachable starting path.
inline void test_tiles(FxTest &test) {
    // Tile frame count follows the generated packed fields, not a record index.
    const uint16_t lastFrame = (maskedFont - tiles) / 32 - 1;
    for (uint8_t caseIndex = 0; caseIndex < 2; ++caseIndex) {
        const uint16_t frame = caseIndex ? lastFrame : 0;
        uint8_t pixels[32];
        FxRead::bytes(tiles + static_cast<uint24_t>(frame) * 32, pixels, sizeof(pixels));
        memset(Arduboy2Base::sBuffer, 0xA5, 1024);
        Blit::draw(-15, -15, 16, 16, tiles, frame, Blit::OVERWRITE);
        // Exactly the bottom-right sprite pixel reaches screen (0,0).
        const uint8_t expected = (0xA5 & 0xFE) | (pixels[31] >> 7);
        test.expectEq(Arduboy2Base::sBuffer[0], expected, F("clipped tile pixel"));
        bool unchanged = true;
        for (uint16_t byte = 1; byte < 1024; ++byte)
            if (Arduboy2Base::sBuffer[byte] != 0xA5) unchanged = false;
        test.expectEq(unchanged, true, F("clipped tile preserves other bytes"));
    }

    const uint16_t floorNorth = rawTileWordAt(12, 6);
    const uint16_t floor = rawTileWordAt(12, 7);
    const uint16_t wallWest = rawTileWordAt(11, 7);
    const uint16_t wallEast = rawTileWordAt(13, 7);
    const uint16_t floorSouth = rawTileWordAt(12, 8);
    const uint16_t emptySouthEast = rawTileWordAt(13, 8);

    test.expectEq(TileProps::tileId(floorNorth), 275, F("north raw tile id"));
    test.expectEq(TileProps::tileProps(floorNorth),
                  static_cast<uint8_t>(TileProps::PROP_WALKABLE | TileProps::PROP_ENCOUNTER),
                  F("north walkable encounter property"));
    test.expectEq(TileProps::tileId(floor), 275, F("spawn raw tile id"));
    test.expectEq(TileProps::hasTileProp(floor, TileProps::PROP_WALKABLE), true,
                  F("spawn walkable property"));
    test.expectEq(TileProps::hasTileProp(floor, TileProps::PROP_ENCOUNTER), true,
                  F("spawn encounter property"));
    test.expectEq(TileProps::tileId(wallWest), 262, F("west wall raw tile id"));
    test.expectEq(TileProps::hasTileProp(wallWest, TileProps::PROP_WALKABLE), false,
                  F("west wall is blocked"));
    test.expectEq(TileProps::tileId(wallEast), 261, F("east wall raw tile id"));
    test.expectEq(TileProps::tileId(floorSouth), 275, F("south corridor raw tile id"));
    test.expectEq(TileProps::tileId(emptySouthEast), 0, F("southeast empty raw tile id"));

    uint8_t storage[TilePropertyWindow::STORAGE_BYTES] = {};
    TilePropertyWindow window(storage);
    window.begin(9, 5);

    uint16_t row[TilePropertyWindow::WINDOW_WIDTH] = {};
    row[3] = floorNorth;
    window.writeRow(1, row);

    memset(row, 0, sizeof(row));
    row[2] = wallWest;
    row[3] = floor;
    row[4] = wallEast;
    window.writeRow(2, row);

    memset(row, 0, sizeof(row));
    row[3] = floorSouth;
    row[4] = emptySouthEast;
    window.writeRow(3, row);

    test.expectEq(WorldCollision::canEnter(12, 6, 256, 256, storage), true,
                  F("walkable north collision"));
    test.expectEq(WorldCollision::canEnter(12, 7, 256, 256, storage), true,
                  F("walkable spawn collision"));
    test.expectEq(WorldCollision::canEnter(11, 7, 256, 256, storage), false,
                  F("west wall collision"));
    test.expectEq(WorldCollision::canEnter(13, 7, 256, 256, storage), false,
                  F("east wall collision"));
    test.expectEq(WorldCollision::canEnter(12, 8, 256, 256, storage), true,
                  F("walkable south corridor collision"));
    test.expectEq(WorldCollision::canEnter(13, 8, 256, 256, storage), false,
                  F("empty tile collision"));
    test.expectEq(WorldCollision::canEnter(-1, 7, 256, 256, storage), false,
                  F("negative map edge collision"));
    test.expectEq(WorldCollision::canEnter(0, 0, 256, 256, storage), false,
                  F("cache miss collision"));

    WorldTransient &world = worldState();
    memcpy(world.propertyWindow, storage, sizeof(storage));
    gameState.playerLocation = static_cast<uint16_t>((6u << 8) | 12u);
    world.motion.directionAndFlags = static_cast<uint8_t>(Direction::DOWN);
    test.expectEq(WorldEngine::moveable(world), true,
                  F("world engine enters walkable target"));

    gameState.playerLocation = static_cast<uint16_t>((7u << 8) | 12u);
    world.motion.directionAndFlags = static_cast<uint8_t>(Direction::LEFT);
    test.expectEq(WorldEngine::moveable(world), false,
                  F("world engine blocks wall target"));

    TilePropertyWindow movementWindow(storage);
    movementWindow.begin(9, 5);
    for (uint8_t rowIndex = 0; rowIndex < TilePropertyWindow::WINDOW_HEIGHT; ++rowIndex) {
        for (uint8_t col = 0; col < TilePropertyWindow::WINDOW_WIDTH; ++col) {
            row[col] = TileProps::packTile(1, TileProps::PROP_WALKABLE);
        }
        movementWindow.writeRow(rowIndex, row);
    }
    memcpy(world.propertyWindow, storage, sizeof(storage));
    gameState.playerLocation = static_cast<uint16_t>((7u << 8) | 12u);
    world.motion.directionAndFlags = 0;
    WorldEngine::inputFromButtons(world, LEFT_BUTTON | RIGHT_BUTTON |
                                         UP_BUTTON | DOWN_BUTTON);
    test.expectEq(world.motion.directionAndFlags & 0x03,
                  static_cast<uint8_t>(Direction::LEFT),
                  F("device snapshot priority selects left"));
    test.expectEq(world.motion.directionAndFlags & 0x04, 0x04,
                  F("device snapshot starts held movement"));

    WorldEngine::inputFromButtons(world, RIGHT_BUTTON | UP_BUTTON | DOWN_BUTTON);
    test.expectEq(world.motion.directionAndFlags & 0x03,
                  static_cast<uint8_t>(Direction::RIGHT),
                  F("device snapshot priority selects right"));
    WorldEngine::inputFromButtons(world, 0);
    test.expectEq(world.motion.directionAndFlags & 0x03,
                  static_cast<uint8_t>(Direction::RIGHT),
                  F("button release keeps facing"));
    test.expectEq(world.motion.directionAndFlags & 0x04, 0,
                  F("button release stops movement"));

    row[4] = TileProps::packTile(2, 0);
    movementWindow.writeRow(2, row);
    memcpy(world.propertyWindow, storage, sizeof(storage));
    world.motion.directionAndFlags = static_cast<uint8_t>(
        static_cast<uint8_t>(Direction::UP) | 0x04 | (3u << 3));
    WorldEngine::inputFromButtons(world, RIGHT_BUTTON);
    test.expectEq(world.motion.directionAndFlags & 0x03,
                  static_cast<uint8_t>(Direction::RIGHT),
                  F("blocked snapshot still changes facing"));
    test.expectEq(world.motion.directionAndFlags & 0x04, 0x04,
                  F("blocked snapshot retains moving state"));
    test.expectEq(world.motion.directionAndFlags & 0x38, 3u << 3,
                  F("blocked snapshot retains prior walk mask"));

    TilePropertyWindow::invalidate(storage);
    test.expectEq(WorldCollision::canEnter(12, 7, 256, 256, storage), false,
                  F("invalidated cache collision"));

    test_map_words(test);
    test_chunk_slots(test);
    test_read_budget(test);
    test_spawn_reachability(test);
}
