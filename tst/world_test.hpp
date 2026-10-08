#pragma once

#include "test.hpp"
#include "tile_props_test.hpp"
#include "../src/engine/world/World.hpp"
#include "../src/engine/world/TileProps.hpp"
#include "../src/engine/world/TilePropertyWindow.hpp"
#include "../src/GameState.hpp"

extern GameState gameState;

static void fillWorldWindow(WorldTransient &world, int16_t originX, int16_t originY,
                            const uint16_t cells[TilePropertyWindow::WINDOW_HEIGHT]
                                               [TilePropertyWindow::WINDOW_WIDTH]) {
    TilePropertyWindow window(world.propertyWindow);
    window.begin(originX, originY);
    for (uint8_t row = 0; row < TilePropertyWindow::WINDOW_HEIGHT; ++row) {
        window.writeRow(row, cells[row]);
    }
}

void WorldTest(TestSuite &suite) {
    Test test(__func__);
    WorldTransient world = {};
    WorldEngine::init(world);
    WorldEngine::loadMap(world, 0, 0);

    WorldEngine::setPos(world, 3, 2);
    test.assert(WorldEngine::location(), static_cast<uint16_t>(0x0203),
                "setPos publishes packed location");
    ViewOffset view = WorldEngine::view(world);
    test.assert(view.x, static_cast<int8_t>(0), "setPos clears x view offset");
    test.assert(view.y, static_cast<int8_t>(0), "setPos clears y view offset");
    test.assert(view.mask, static_cast<uint8_t>(0), "setPos clears walk mask");

    WorldEngine::beginMoveForTest(world, Direction::RIGHT);
    for (uint8_t i = 0; i < 8; ++i) WorldEngine::moveChar(world);
    view = WorldEngine::view(world);
    test.assert(view.x, static_cast<int8_t>(-8), "mid-step view offset is signed");
    test.assert(view.mask, static_cast<uint8_t>(0b01000000), "mid-step retains direction mask");
    test.assert(WorldEngine::location(), static_cast<uint16_t>(0x0203),
                "logical position stays put during animation");

    for (uint8_t i = 0; i < 8; ++i) WorldEngine::moveChar(world);
    test.assert(WorldEngine::location(), static_cast<uint16_t>(0x0204),
                "completed move publishes one tile");
    view = WorldEngine::view(world);
    test.assert(view.x, static_cast<int8_t>(0), "completed move clears x offset");
    test.assert(view.y, static_cast<int8_t>(0), "completed move clears y offset");
    test.assert(view.mask, static_cast<uint8_t>(0), "completed move clears walk mask");

    gameState.playerLocation = 0x0408;
    WorldEngine::syncFromLocation(world);
    test.assert(WorldEngine::location(), static_cast<uint16_t>(0x0408),
                "sync retains externally published teleport");
    view = WorldEngine::view(world);
    test.assert(view.x, static_cast<int8_t>(0), "teleport sync clears x offset");
    test.assert(view.y, static_cast<int8_t>(0), "teleport sync clears y offset");
    test.assert(view.mask, static_cast<uint8_t>(0), "teleport sync clears walk mask");

    suite.addTest(test);
}

void WorldCollisionTest(TestSuite &suite) {
    Test test(__func__);
    WorldTransient world = {};
    WorldEngine::init(world);
    WorldEngine::setPos(world, 3, 2);

    uint16_t cells[TilePropertyWindow::WINDOW_HEIGHT][TilePropertyWindow::WINDOW_WIDTH];
    for (uint8_t row = 0; row < TilePropertyWindow::WINDOW_HEIGHT; ++row) {
        for (uint8_t col = 0; col < TilePropertyWindow::WINDOW_WIDTH; ++col) {
            cells[row][col] = TileProps::packTile(1, TileProps::PROP_WALKABLE);
        }
    }
    fillWorldWindow(world, 0, 0, cells);

    const Direction directions[] = {
        Direction::UP, Direction::RIGHT, Direction::DOWN, Direction::LEFT
    };
    for (Direction direction : directions) {
        WorldEngine::beginMoveForTest(world, direction);
        test.assert(WorldEngine::moveable(world), true,
                    "loaded walkable neighbor can be entered in every direction");
    }

    WorldEngine::setPos(world, 3, 2);
    test.assert(WorldEngine::moveable(world), false,
                "invalidated cache blocks a movement query");

    cells[2][4] = TileProps::packTile(0, TileProps::PROP_WALKABLE);
    fillWorldWindow(world, 0, 0, cells);
    WorldEngine::beginMoveForTest(world, Direction::RIGHT);
    test.assert(WorldEngine::moveable(world), false,
                "empty GID blocks movement even if property bits are inconsistent");
    cells[2][4] = TileProps::packTile(2, 0);
    fillWorldWindow(world, 0, 0, cells);
    test.assert(WorldEngine::moveable(world), false,
                "non-walkable wall blocks movement");
    cells[2][4] = TileProps::packTile(3, TileProps::PROP_WATER);
    fillWorldWindow(world, 0, 0, cells);
    test.assert(WorldEngine::moveable(world), false,
                "water without walkable property blocks movement");
    cells[2][4] = TileProps::packTile(4, TileProps::PROP_WALKABLE);
    fillWorldWindow(world, 0, 0, cells);
    test.assert(WorldEngine::moveable(world), true,
                "walkable land permits movement");

    for (uint8_t row = 0; row < TilePropertyWindow::WINDOW_HEIGHT; ++row) {
        for (uint8_t col = 0; col < TilePropertyWindow::WINDOW_WIDTH; ++col) {
            cells[row][col] = TileProps::packTile(1, TileProps::PROP_WALKABLE);
        }
    }
    WorldEngine::setPos(world, 0, 0);
    fillWorldWindow(world, -3, -2, cells);
    WorldEngine::beginMoveForTest(world, Direction::LEFT);
    test.assert(WorldEngine::moveable(world), false, "left map edge blocks movement");
    WorldEngine::beginMoveForTest(world, Direction::UP);
    test.assert(WorldEngine::moveable(world), false, "top map edge blocks movement");
    WorldEngine::beginMoveForTest(world, Direction::RIGHT);
    test.assert(WorldEngine::moveable(world), true, "rightward movement stays in map at origin");
    WorldEngine::beginMoveForTest(world, Direction::DOWN);
    test.assert(WorldEngine::moveable(world), true, "downward movement stays in map at origin");

    WorldEngine::setPos(world, 255, 255);
    fillWorldWindow(world, 252, 253, cells);
    WorldEngine::beginMoveForTest(world, Direction::RIGHT);
    test.assert(WorldEngine::moveable(world), false, "right map edge blocks movement");
    WorldEngine::beginMoveForTest(world, Direction::DOWN);
    test.assert(WorldEngine::moveable(world), false, "bottom map edge blocks movement");
    WorldEngine::beginMoveForTest(world, Direction::LEFT);
    test.assert(WorldEngine::moveable(world), true, "leftward movement stays in map at far edge");
    WorldEngine::beginMoveForTest(world, Direction::UP);
    test.assert(WorldEngine::moveable(world), true, "upward movement stays in map at far edge");
    suite.addTest(test);
}

void WorldPlayerFrameTest(TestSuite &suite) {
    Test test("World player frame selection");
    WorldTransient world{};
    for (uint8_t d = 0; d < 4; ++d) {
        const Direction facing = static_cast<Direction>(d);
        world.motion.directionAndFlags = d;
        test.assert(WorldEngine::playerFrame(world), static_cast<uint8_t>(d * 3), "idle follows facing");
        WorldEngine::setPos(world, 10, 10);
        WorldEngine::beginMoveForTest(world, facing);
        for (uint8_t step = 1; step < 16; ++step) {
            WorldEngine::moveChar(world);
            test.assert(WorldEngine::playerFrame(world), static_cast<uint8_t>(d * 3 + (step < 8 ? 1 : 2)), "gait follows actual camera step");
        }
        WorldEngine::moveChar(world);
        test.assert(WorldEngine::playerFrame(world), static_cast<uint8_t>(d * 3), "completed step resumes directional idle");
    }
    suite.addTest(test);
}

void WorldSuite(TestRunner &runner) {
    TestSuite suite("World Suite");
    TilePropsEncodingTest(suite);
    TilePropertyWindowTest(suite);
    TileCollisionHelperTest(suite);
    WorldTest(suite);
    WorldCollisionTest(suite);
    WorldPlayerFrameTest(suite);
    runner.addTestSuite(suite);
}
