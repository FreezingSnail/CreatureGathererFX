#pragma once

#include <string.h>

#include "test.hpp"
#include "host/Arduboy2.h"
#include "src/FXDataFake.hpp"

#include "../src/engine/world/StepEvent.hpp"
#include "../src/engine/world/TilePropertyWindow.hpp"
#include "../src/engine/world/World.hpp"
#include "../src/engine/world/LureZone.hpp"
#include "../src/engine/world/Encounter.hpp"
#include "../src/fxdata.h"
#include "../src/globals.hpp"

namespace worldMovementFake {
void reset();
void request(Direction direction);
void requestButtons(uint8_t buttons);
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

namespace lureStepDetail {
uint8_t forcedRoll = 0;
uint8_t forcedRng(uint8_t, uint8_t) { return forcedRoll; }

// Every window cell is walkable and encounter-tagged so the step hook reaches
// the roll gate at the destination tile.
void fillEncounterWindow(WorldTransient &world, uint8_t x, uint8_t y) {
    uint16_t cells[TilePropertyWindow::WINDOW_HEIGHT][TilePropertyWindow::WINDOW_WIDTH];
    for (uint8_t row = 0; row < TilePropertyWindow::WINDOW_HEIGHT; ++row) {
        for (uint8_t col = 0; col < TilePropertyWindow::WINDOW_WIDTH; ++col) {
            cells[row][col] = TileProps::packTile(
                1, TileProps::PROP_WALKABLE | TileProps::PROP_ENCOUNTER);
        }
    }
    TilePropertyWindow window(world.propertyWindow);
    window.begin(static_cast<int16_t>(x) - 3, static_cast<int16_t>(y) - 2);
    for (uint8_t row = 0; row < TilePropertyWindow::WINDOW_HEIGHT; ++row) {
        window.writeRow(row, cells[row]);
    }
}

// Seed a valid inert zone cache so select() succeeds without any FX read.
void seedInertZone(WorldTransient &world) {
    uint8_t *cache = world.zoneTableCache;
    memset(cache, 0, Encounter::CACHE_BYTES);
    for (uint8_t slot = 0; slot < Encounter::SLOT_COUNT; ++slot) cache[slot] = slot;
    cache[Encounter::SLOT_COUNT] = 2;
    cache[Encounter::SLOT_COUNT + 1] = 5;
    cache[Encounter::CACHE_VALID_OFFSET] = 1;
    cache[Encounter::CACHE_TABLE_BASE_OFFSET] = 0;
    cache[Encounter::CACHE_TABLE_COUNT_OFFSET] = 1;
    cache[Encounter::CACHE_TABLE_INDEX_OFFSET] = 0;
}

// Author chunk 1 and chunk 33 via the FX fake so onChunkChange's load() result
// drives the lure keep/clear decision.
void prepareLureZones() {
    fxDataFake::dataBase = EncounterFXData::zoneDefs;
    std::memset(fxDataFake::dataBytes, 0, sizeof(fxDataFake::dataBytes));
    const uint16_t chunks[2] = {1, 33};
    for (uint8_t zone = 0; zone < 2; ++zone) {
        const uint8_t o = static_cast<uint8_t>(zone * Encounter::ZONE_BYTES);
        fxDataFake::dataBytes[o] = static_cast<uint8_t>(chunks[zone]);
        fxDataFake::dataBytes[o + 1] = static_cast<uint8_t>(chunks[zone] >> 8);
        fxDataFake::dataBytes[o + 2] = 0;
        fxDataFake::dataBytes[o + 3] = Encounter::TABLE_COUNT;
        fxDataFake::dataBytes[o + 4] = 1;
        fxDataFake::dataBytes[o + 5] = 1;
        fxDataFake::dataBytes[o + 6] = 2;
        fxDataFake::dataBytes[o + 7] = 3;
        fxDataFake::dataBytes[o + 8] = 4;
        fxDataFake::dataBytes[o + 9] = 0;
    }
    for (uint8_t zone = 2; zone < Encounter::ZONE_COUNT; ++zone) {
        const uint8_t o = static_cast<uint8_t>(zone * Encounter::ZONE_BYTES);
        fxDataFake::dataBytes[o] = 255;
        fxDataFake::dataBytes[o + 1] = 255;
    }
    const uint8_t tableOffset =
        static_cast<uint8_t>(Encounter::ZONE_COUNT * Encounter::ZONE_BYTES);
    for (uint8_t table = 0; table < Encounter::TABLE_COUNT; ++table) {
        const uint8_t base = static_cast<uint8_t>(
            tableOffset + table * Encounter::TABLE_BYTES);
        for (uint8_t slot = 0; slot < Encounter::SLOT_COUNT; ++slot)
            fxDataFake::dataBytes[base + slot] = slot;
        fxDataFake::dataBytes[base + Encounter::SLOT_COUNT] = 2;
        fxDataFake::dataBytes[base + Encounter::SLOT_COUNT + 1] = 5;
    }
    fxDataFake::readCount = 0;
}
} // namespace lureStepDetail

void LureStepTest(TestSuite &suite) {
    using namespace lureStepDetail;
    Test test(__func__);
    StepEventTest::setEncounterRng(forcedRng);

    // Decay and expiry through the step hook.
    gameState.state = GameState_t::WORLD;
    gameState.playerLocation = 0x0209;
    TilePropertyWindow::invalidate(worldState().propertyWindow);
    LureZone::clear();
    LureZone::deposit(LureZone::lureId(0, 0));
    const uint8_t charge = LureZone::charge();
    test.assert(charge, static_cast<uint8_t>(4), "tier-zero deposit charge");
    for (uint8_t step = 0; step + 1 < charge; ++step) onStep(0x0209);
    test.assert(LureZone::charge(), static_cast<uint8_t>(1),
                "step hook decays one charge per step");
    test.assert(LureZone::active(), true, "lure stays active before expiry");
    onStep(0x0209);
    test.assert(LureZone::active(), false, "step hook expires the lure");

    // 25% gate: a high roll refuses, a low roll enters battle.
    player.basic();
    for (uint8_t slot = 1; slot < PARTY_SIZE; ++slot) player.party[slot].level = 0;
    player.party[0].level = 5;
    seedInertZone(worldState());
    fillEncounterWindow(worldState(), 9, 2);
    gameState.state = GameState_t::WORLD;
    gameState.playerLocation = 0x0209;
    forcedRoll = 99;
    onStep(0x0209);
    test.assert(gameState.state, GameState_t::WORLD,
                "encounter roll above 25% is refused");
    forcedRoll = 0;
    onStep(0x0209);
    test.assert(gameState.state, GameState_t::BATTLE,
                "encounter roll below 25% enters battle");
    exitBattle();

    // A lure belongs to the authored zones: crossing the pocket's chunk 1 and
    // chunk 33 boundary keeps it, while a chunk with no authored zone clears it.
    prepareLureZones();
    gameState.state = GameState_t::WORLD;
    gameState.playerLocation = 0x0209;
    LureZone::clear();
    LureZone::deposit(LureZone::lureId(1, 0));
    test.assert(LureZone::zoneChunk(), Chunk::chunkOfLocation(0x0209),
                "lure records its deposit chunk");
    WorldEngine::setPos(worldState(), 9, 4);
    test.assert(Chunk::chunkOfLocation(gameState.playerLocation),
                static_cast<uint16_t>(33), "crossing lands in chunk 33");
    test.assert(LureZone::active(), true, "lure survives the chunk 1 -> 33 crossing");
    WorldEngine::setPos(worldState(), 16, 2);
    test.assert(Chunk::chunkOfLocation(gameState.playerLocation),
                static_cast<uint16_t>(2), "crossing lands in an unauthored chunk");
    test.assert(LureZone::active(), false, "unauthored chunk clears the lure");

    suite.addTest(test);
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

    const uint8_t allDirections = LEFT_BUTTON | RIGHT_BUTTON | UP_BUTTON | DOWN_BUTTON;
    worldMovementFake::requestButtons(allDirections);
    WorldEngine::input(worldState());
    test.assert(worldState().motion.directionAndFlags & 0x03,
                static_cast<uint8_t>(Direction::LEFT),
                "simultaneous buttons choose left before other directions");
    test.assert(worldState().motion.directionAndFlags & 0x04, static_cast<uint8_t>(0x04),
                "held direction starts movement through shared update path");

    worldMovementFake::requestButtons(RIGHT_BUTTON | UP_BUTTON | DOWN_BUTTON);
    WorldEngine::input(worldState());
    test.assert(worldState().motion.directionAndFlags & 0x03,
                static_cast<uint8_t>(Direction::RIGHT),
                "right has priority over held up and down");
    worldMovementFake::requestButtons(UP_BUTTON | DOWN_BUTTON);
    WorldEngine::input(worldState());
    test.assert(worldState().motion.directionAndFlags & 0x03,
                static_cast<uint8_t>(Direction::UP), "up has priority over held down");
    worldMovementFake::requestButtons(DOWN_BUTTON);
    WorldEngine::input(worldState());
    test.assert(worldState().motion.directionAndFlags & 0x03,
                static_cast<uint8_t>(Direction::DOWN), "down remains available alone");

    const uint8_t downFlags = worldState().motion.directionAndFlags;
    WorldEngine::input(worldState());
    test.assert(worldState().motion.directionAndFlags & 0x03,
                static_cast<uint8_t>(Direction::DOWN), "release preserves facing");
    test.assert(worldState().motion.directionAndFlags & 0x04, static_cast<uint8_t>(0),
                "release stops movement");
    test.assert(worldState().motion.directionAndFlags & 0x38, downFlags & 0x38,
                "release preserves the prior walk mask");

    worldState().motion.directionAndFlags = static_cast<uint8_t>(
        static_cast<uint8_t>(Direction::UP) | 0x04 | (3u << 3));
    fillStepWindow(worldState(), 3, 2, true);
    worldMovementFake::requestButtons(RIGHT_BUTTON);
    WorldEngine::input(worldState());
    test.assert(worldState().motion.directionAndFlags & 0x03,
                static_cast<uint8_t>(Direction::RIGHT),
                "blocked request still changes facing");
    test.assert(worldState().motion.directionAndFlags & 0x04, static_cast<uint8_t>(0x04),
                "blocked request retains existing moving state");
    test.assert(worldState().motion.directionAndFlags & 0x38, static_cast<uint8_t>(3u << 3),
                "blocked request retains previous walk mask");

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
    LureStepTest(suite);
    runner.addTestSuite(suite);
}
