#pragma once

#include "test.hpp"
#include "../src/engine/world/World.hpp"
#include "../src/GameState.hpp"

extern GameState gameState;

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

void WorldSuite(TestRunner &runner) {
    TestSuite suite("World Suite");
    WorldTest(suite);
    runner.addTestSuite(suite);
}
