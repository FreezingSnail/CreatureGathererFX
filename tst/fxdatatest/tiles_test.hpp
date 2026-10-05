#pragma once

#include <string.h>

#include "fxtest.hpp"
#include "src/GameState.hpp"
#include "src/engine/world/TilePropertyWindow.hpp"
#include "src/engine/world/World.hpp"
#include "src/engine/world/WorldCollision.hpp"
#include "src/fxdata.h"
#include "src/lib/ReadData.hpp"
#include "src/lib/FxRead.hpp"

extern GameState gameState;

namespace {

uint16_t rawTileWordAt(uint8_t x, uint8_t y) {
    const uint32_t offset =
        (static_cast<uint32_t>(y) * 256u + x) * sizeof(uint16_t);
    const uint24_t address = raw_map_data + static_cast<uint24_t>(offset);
    return ReadFXu16(address);
}

} // namespace

// The configured real map reaches GID 298, so the device image can validate
// its authored property bits and collision path only with those real entries.
// Higher GIDs such as 527 are covered by the generator and host tests.
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
    const uint16_t emptySouth = rawTileWordAt(12, 8);

    test.expectEq(TileProps::tileId(floorNorth), 275, F("north raw tile id"));
    test.expectEq(TileProps::tileProps(floorNorth), TileProps::PROP_WALKABLE,
                  F("north walkable property"));
    test.expectEq(TileProps::tileId(floor), 275, F("spawn raw tile id"));
    test.expectEq(TileProps::hasTileProp(floor, TileProps::PROP_WALKABLE), true,
                  F("spawn walkable property"));
    test.expectEq(TileProps::tileId(wallWest), 262, F("west wall raw tile id"));
    test.expectEq(TileProps::hasTileProp(wallWest, TileProps::PROP_WALKABLE), false,
                  F("west wall is blocked"));
    test.expectEq(TileProps::tileId(wallEast), 261, F("east wall raw tile id"));
    test.expectEq(TileProps::tileId(emptySouth), 0, F("south empty raw tile id"));

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
    row[3] = emptySouth;
    window.writeRow(3, row);

    test.expectEq(WorldCollision::canEnter(12, 6, 256, 256, storage), true,
                  F("walkable north collision"));
    test.expectEq(WorldCollision::canEnter(12, 7, 256, 256, storage), true,
                  F("walkable spawn collision"));
    test.expectEq(WorldCollision::canEnter(11, 7, 256, 256, storage), false,
                  F("west wall collision"));
    test.expectEq(WorldCollision::canEnter(13, 7, 256, 256, storage), false,
                  F("east wall collision"));
    test.expectEq(WorldCollision::canEnter(12, 8, 256, 256, storage), false,
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
}
