#pragma once

#include <stdint.h>
#include <string.h>

#include "test.hpp"
#include "../src/engine/world/TileProps.hpp"
#include "../src/engine/world/TilePropertyWindow.hpp"
#include "../src/engine/world/WorldCollision.hpp"

void TilePropsEncodingTest(TestSuite &suite) {
    Test test(__func__);
    const uint16_t words[] = {
        TileProps::packTile(0, 0),
        TileProps::packTile(527, TileProps::PROP_WALKABLE),
        TileProps::packTile(1023, TileProps::TILE_PROP_BITS),
    };
    test.assert(TileProps::tileId(words[0]), static_cast<uint16_t>(0),
                "empty GID remains zero");
    test.assert(TileProps::tileId(words[1]), static_cast<uint16_t>(527),
                "tile ID preserves bit nine");
    test.assert(TileProps::tileProps(words[1]), TileProps::PROP_WALKABLE,
                "property bits start above the low ten GID bits");
    test.assert(words[1], static_cast<uint16_t>(0x060F),
                "packed GID 527 and walkable bit use the fixed map word ABI");
    test.assert(TileProps::tileId(words[2]), static_cast<uint16_t>(1023),
                "maximum tile ID fills only the low ten bits");
    test.assert(TileProps::tileProps(words[2]), TileProps::TILE_PROP_BITS,
                "all six property bits fit the high map word bits");
    test.assert(TileProps::isTileIdValid(1023), true, "maximum tile ID is accepted");
    test.assert(TileProps::isTileIdValid(1024), false, "out-of-range tile ID is rejected");

    for (uint8_t properties = 0; properties <= TileProps::TILE_PROP_BITS; ++properties) {
        const uint16_t packed = TileProps::packTile(527, properties);
        test.assert(TileProps::tileId(packed), static_cast<uint16_t>(527),
                    "property combination preserves tile ID");
        test.assert(TileProps::tileProps(packed), properties,
                    "property combination round-trips all six bits");
    }
    test.assert(TileProps::hasTileProp(words[1], TileProps::PROP_WALKABLE), true,
                "walkable property query reads its bit");
    test.assert(TileProps::hasTileProp(words[1], TileProps::PROP_WATER), false,
                "water property query leaves unrelated bits clear");
    suite.addTest(test);
}

void TilePropertyWindowTest(TestSuite &suite) {
    Test test(__func__);
    uint8_t storage[TilePropertyWindow::STORAGE_BYTES];
    memset(storage, 0xFF, sizeof(storage));
    TilePropertyWindow::invalidate(storage);
    TilePropertyWindow window(storage);
    uint8_t properties = 0;
    bool occupied = true;
    test.assert(window.lookup(0, 0, properties, occupied), false,
                "invalidated window rejects cache misses");

    window.begin(-3, -2);
    uint16_t row[TilePropertyWindow::WINDOW_WIDTH];
    for (uint8_t col = 0; col < TilePropertyWindow::WINDOW_WIDTH; ++col) {
        row[col] = TileProps::packTile(static_cast<uint16_t>(col),
                                      static_cast<uint8_t>(col & TileProps::TILE_PROP_BITS));
    }
    row[0] = TileProps::packTile(0, TileProps::PROP_WALKABLE | TileProps::PROP_SPARE);
    window.writeRow(0, row);
    test.assert(window.lookup(-3, -2, properties, occupied), true,
                "loaded row covers its signed origin");
    test.assert(properties, static_cast<uint8_t>(TileProps::PROP_WALKABLE | TileProps::PROP_SPARE),
                "window preserves every property bit for empty GID");
    test.assert(occupied, false, "window tracks GID zero separately from properties");
    test.assert(window.lookup(5, -2, properties, occupied), true,
                "loaded row reaches the ninth cell");
    test.assert(properties, static_cast<uint8_t>(8), "packed cells retain the last property mask");
    test.assert(occupied, true, "nonzero GID marker survives packed cell boundary");
    test.assert(window.lookup(-3, -1, properties, occupied), false,
                "unloaded row remains a cache miss");
    test.assert(window.lookup(6, -2, properties, occupied), false,
                "coordinate past window width is a cache miss");

    for (uint8_t r = 1; r < TilePropertyWindow::WINDOW_HEIGHT; ++r) {
        for (uint8_t c = 0; c < TilePropertyWindow::WINDOW_WIDTH; ++c) {
            const uint8_t mask = static_cast<uint8_t>((r * 9 + c) & TileProps::TILE_PROP_BITS);
            row[c] = TileProps::packTile(1023, mask);
        }
        window.writeRow(r, row);
    }
    test.assert(window.lookup(5, 2, properties, occupied), true,
                "window includes its final row and column");
    test.assert(properties, static_cast<uint8_t>(44 & TileProps::TILE_PROP_BITS),
                "six-bit stream preserves the final cell property mask");
    test.assert(occupied, true, "final cell retains tile presence");
    suite.addTest(test);
}

void TileCollisionHelperTest(TestSuite &suite) {
    Test test(__func__);
    uint8_t storage[TilePropertyWindow::STORAGE_BYTES] = {};
    TilePropertyWindow window(storage);
    window.begin(0, 0);

    uint16_t row[TilePropertyWindow::WINDOW_WIDTH] = {};
    row[0] = TileProps::packTile(0, TileProps::PROP_WALKABLE);
    row[1] = TileProps::packTile(1, TileProps::PROP_WALKABLE);
    row[2] = TileProps::packTile(2, TileProps::PROP_WATER);
    row[3] = TileProps::packTile(3, 0);
    window.writeRow(0, row);

    test.assert(WorldCollision::canEnter(0, 0, 256, 256, storage), false,
                "empty GID stays blocked even with inconsistent walkable metadata");
    test.assert(WorldCollision::canEnter(1, 0, 256, 256, storage), true,
                "walkable land allows entry");
    test.assert(WorldCollision::canEnter(2, 0, 256, 256, storage), false,
                "water without walkable property blocks entry");
    test.assert(WorldCollision::canEnter(3, 0, 256, 256, storage), false,
                "non-walkable wall blocks entry");
    test.assert(WorldCollision::canEnter(1, 1, 256, 256, storage), false,
                "unloaded cache row blocks entry");
    test.assert(WorldCollision::canEnter(-1, 0, 256, 256, storage), false,
                "negative map coordinate blocks entry");
    test.assert(WorldCollision::canEnter(256, 0, 256, 256, storage), false,
                "coordinate at map width blocks entry");
    window.invalidate(storage);
    test.assert(WorldCollision::canEnter(1, 0, 256, 256, storage), false,
                "invalid cache blocks entry");
    suite.addTest(test);
}

void TilePropsSuite(TestRunner &runner) {
    TestSuite suite("Tile Properties Suite");
    TilePropsEncodingTest(suite);
    TilePropertyWindowTest(suite);
    TileCollisionHelperTest(suite);
    runner.addTestSuite(suite);
}
