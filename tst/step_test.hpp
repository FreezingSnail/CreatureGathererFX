#pragma once

#include "test.hpp"

#include "../src/engine/world/StepEvent.hpp"
#include "../src/engine/world/TilePropertyWindow.hpp"
#include "../src/engine/world/World.hpp"
#include "../src/globals.hpp"

namespace worldMovementFake {
void reset();
void request(Direction direction);
uint16_t inputCalls();
}

static void fillStepWindow(WorldTransient &world, uint8_t x, uint8_t y,
                           bool blockRight = false) {
    uint16_t cells[TilePropertyWindow::WINDOW_HEIGHT][TilePropertyWindow::WINDOW_WIDTH];
    for (uint8_t row = 0; row < TilePropertyWindow::WINDOW_HEIGHT; ++row) {
        for (uint8_t col = 0; col < TilePropertyWindow::WINDOW_WIDTH; ++col) {
            cells[row][col] = TileProps::packTile(1, TileProps::PROP_WALKABLE);
        }
    }
    if (blockRight) cells[2][4] = TileProps::packTile(2, 0);

    TilePropertyWindow window(world.propertyWindow);
    window.begin(static_cast<int16_t>(x) - 3, static_cast<int16_t>(y) - 2);
    for (uint8_t row = 0; row < TilePropertyWindow::WINDOW_HEIGHT; ++row) {
        window.writeRow(row, cells[row]);
    }
}

void StepSuite(TestRunner &runner) {
    TestSuite suite("Step Events");
    Test test(__func__);
    dialogMenu.clear();
    worldMovementFake::reset();
    plants = PlantGameState{};
    gameState.playerLocation = 0x0203;
    WorldEngine::init(worldState());
    fillStepWindow(worldState(), 3, 2);

    worldMovementFake::request(Direction::RIGHT);
    for (uint8_t frame = 0; frame < 15; ++frame) WorldEngine::runMap(worldState());
    test.assert(gameState.playerLocation, static_cast<uint16_t>(0x0203),
                "partial movement does not publish the destination");
    test.assert(plants.ticker, static_cast<uint8_t>(0),
                "partial movement does not dispatch a step event");

    WorldEngine::runMap(worldState());
    test.assert(gameState.playerLocation, static_cast<uint16_t>(0x0204),
                "completed movement publishes the destination tile");
    test.assert(plants.ticker, static_cast<uint8_t>(1),
                "completed movement dispatches one step event");
    for (uint8_t frame = 0; frame < 32; ++frame) WorldEngine::runMap(worldState());
    test.assert(plants.ticker, static_cast<uint8_t>(1),
                "stationary frames do not dispatch duplicate step events");

    plants = PlantGameState{};
    gameState.playerLocation = 0x0203;
    WorldEngine::init(worldState());
    fillStepWindow(worldState(), 3, 2, true);
    worldMovementFake::reset();
    worldMovementFake::request(Direction::RIGHT);
    for (uint8_t frame = 0; frame < 20; ++frame) WorldEngine::runMap(worldState());
    test.assert(gameState.playerLocation, static_cast<uint16_t>(0x0203),
                "blocked movement leaves the player in place");
    test.assert(plants.ticker, static_cast<uint8_t>(0),
                "blocked movement does not dispatch a step event");

    plants = PlantGameState{};
    gameState.playerLocation = 0x0203;
    WorldEngine::init(worldState());
    fillStepWindow(worldState(), 3, 2);
    worldMovementFake::reset();
    worldMovementFake::request(Direction::RIGHT);
    PopUpDialog active = {};
    active.type = SCRIPT_TEXT;
    active.textAddress = 1;
    dialogMenu.pushMenu(active);
    WorldEngine::runMap(worldState());
    test.assert(worldMovementFake::inputCalls(), static_cast<uint16_t>(0),
                "dialog-gated input does not reach the movement input handler");
    test.assert(gameState.playerLocation, static_cast<uint16_t>(0x0203),
                "dialog-gated input does not move the player");
    test.assert(plants.ticker, static_cast<uint8_t>(0),
                "dialog-gated input does not dispatch a step event");
    dialogMenu.clear();

    plants = PlantGameState{};
    for (uint8_t step = 0; step < 127; ++step) onStep(0x0204);
    test.assert(plants.ticker, static_cast<uint8_t>(127),
                "plant ticker advances once per step before rollover");
    test.assert(plants.plantStages.getStage(0), static_cast<uint8_t>(0),
                "plant stage stays unchanged before the 128th step");
    onStep(0x0204);
    test.assert(plants.ticker, static_cast<uint8_t>(0),
                "plant ticker resets on the 128th step");
    test.assert(plants.plantStages.getStage(0), static_cast<uint8_t>(1),
                "128th step advances plant stages");

    suite.addTest(test);
    runner.addTestSuite(suite);
}
