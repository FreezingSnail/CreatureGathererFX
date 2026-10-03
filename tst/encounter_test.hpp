#pragma once

#include <cstring>

#include "test.hpp"
#include "../src/engine/world/Encounter.hpp"
#include "../src/engine/world/StepEvent.hpp"
#include "../src/engine/world/TilePropertyWindow.hpp"
#include "../src/engine/world/World.hpp"
#include "../src/engine/ModeState.hpp"
#include "../src/fxdata.h"
#include "../src/globals.hpp"
#include "src/FXDataFake.hpp"

namespace encounter_test_detail {

uint8_t forcedRoll = 0;
uint8_t rngCalls = 0;
uint8_t lastLower = 0;
uint8_t lastUpper = 0;

uint8_t forcedRng(uint8_t lower, uint8_t upper) {
    ++rngCalls;
    lastLower = lower;
    lastUpper = upper;
    return forcedRoll;
}

void clearParty() {
    for (uint8_t slot = 0; slot < PARTY_SIZE; ++slot) {
        player.party[slot] = Creature();
        player.creatureHPs[slot] = 0;
    }
}

void prepareParty(uint8_t first, uint8_t second, uint8_t third) {
    clearParty();
    player.basic();
    player.party[0].level = first;
    player.party[1].level = second;
    player.party[2].level = third;
    player.creatureHPs[0] = player.party[0].statlist.hp;
    player.creatureHPs[1] = player.party[1].statlist.hp;
    player.creatureHPs[2] = player.party[2].statlist.hp;
}

void prepareFx(uint16_t chunkId, int8_t levelOffset = 0,
               uint8_t tableBase = 0, uint8_t tableCount = 1) {
    fxDataFake::dataBase = EncounterFXData::zoneDefs;
    std::memset(fxDataFake::dataBytes, 0, sizeof(fxDataFake::dataBytes));
    fxDataFake::readCount = 0;
    const uint16_t zoneOffset = static_cast<uint16_t>(0);
    fxDataFake::dataBytes[zoneOffset] = static_cast<uint8_t>(chunkId);
    fxDataFake::dataBytes[zoneOffset + 1] = static_cast<uint8_t>(chunkId >> 8);
    fxDataFake::dataBytes[zoneOffset + 2] = tableBase;
    fxDataFake::dataBytes[zoneOffset + 3] = tableCount;
    fxDataFake::dataBytes[zoneOffset + 4] = 1;
    fxDataFake::dataBytes[zoneOffset + 9] = static_cast<uint8_t>(levelOffset);
    for (uint8_t zone = 1; zone < Encounter::ZONE_COUNT; ++zone) {
        const uint8_t offset = static_cast<uint8_t>(zone * Encounter::ZONE_BYTES);
        fxDataFake::dataBytes[offset] = 255;
        fxDataFake::dataBytes[offset + 1] = 255;
    }
    const uint8_t tableOffset = Encounter::ZONE_COUNT * Encounter::ZONE_BYTES;
    for (uint8_t slot = 0; slot < Encounter::SLOT_COUNT; ++slot) {
        fxDataFake::dataBytes[tableOffset + slot] = slot;
    }
    fxDataFake::dataBytes[tableOffset + Encounter::SLOT_COUNT] = 1;
    fxDataFake::dataBytes[tableOffset + Encounter::SLOT_COUNT + 1] = 31;
}

void fillProperties(WorldTransient &world, uint8_t x, uint8_t y,
                    bool encounter) {
    uint16_t cells[TilePropertyWindow::WINDOW_HEIGHT]
                  [TilePropertyWindow::WINDOW_WIDTH];
    const uint8_t properties = static_cast<uint8_t>(
        TileProps::PROP_WALKABLE |
        (encounter ? TileProps::PROP_ENCOUNTER : 0));
    for (uint8_t row = 0; row < TilePropertyWindow::WINDOW_HEIGHT; ++row) {
        for (uint8_t col = 0; col < TilePropertyWindow::WINDOW_WIDTH; ++col) {
            cells[row][col] = TileProps::packTile(1, properties);
        }
    }
    TilePropertyWindow window(world.propertyWindow);
    window.begin(0, 0);
    for (uint8_t row = 0; row < TilePropertyWindow::WINDOW_HEIGHT; ++row) {
        window.writeRow(row, cells[row]);
    }
    (void)x;
    (void)y;
}

void prepareWorld(uint16_t location, bool encounter) {
    gameState.playerLocation = location;
    gameState.state = GameState_t::WORLD;
    prepareParty(9, 6, 0);
    prepareFx(Chunk::chunkOfLocation(location));
    WorldEngine::init(worldState());
    WorldEngine::loadMap(worldState(), 0, 0);
    fillProperties(worldState(), static_cast<uint8_t>(location),
                   static_cast<uint8_t>(location >> 8), encounter);
    forcedRoll = 0;
    rngCalls = 0;
    lastLower = 0;
    lastUpper = 0;
    StepEventTest::setEncounterRng(forcedRng);
}

void seedCache(uint8_t *cache, int8_t offset, uint8_t floor, uint8_t ceil) {
    std::memset(cache, 0, Encounter::CACHE_BYTES);
    for (uint8_t slot = 0; slot < Encounter::SLOT_COUNT; ++slot) {
        cache[slot] = slot;
    }
    cache[Encounter::SLOT_COUNT] = floor;
    cache[Encounter::SLOT_COUNT + 1] = ceil;
    cache[Encounter::CACHE_VALID_OFFSET] = 1;
    cache[Encounter::CACHE_TABLE_BASE_OFFSET] = 0;
    cache[Encounter::CACHE_TABLE_COUNT_OFFSET] = 1;
    cache[Encounter::CACHE_LEVEL_OFFSET] = static_cast<uint8_t>(offset);
}

} // namespace encounter_test_detail

void EncounterDecisionTest(TestSuite &suite) {
    using namespace encounter_test_detail;
    Test test(__func__);
    Creature party[PARTY_SIZE] = {};
    for (uint8_t slot = 0; slot < PARTY_SIZE; ++slot) {
        party[slot].level = 0;
    }
    party[0].level = 9;
    party[1].level = 6;
    test.assert(Encounter::partyAverageLevel(party), static_cast<uint8_t>(5),
                "empty slot keeps the three-slot divisor");

    uint8_t cache[Encounter::CACHE_BYTES];
    seedCache(cache, 0, 2, 8);
    Encounter::Decision decision = {};
    forcedRoll = 0;
    rngCalls = 0;
    test.assert(Encounter::select(cache, party, forcedRng, decision), true,
                "ordinary cached table selects a wild creature");
    test.assert(decision.slot, static_cast<uint8_t>(0), "slot zero boundary");
    test.assert(decision.creatureId, static_cast<uint8_t>(0), "slot zero ID");
    test.assert(decision.level, static_cast<uint8_t>(5), "ordinary level");
    test.assert(rngCalls, static_cast<uint8_t>(1), "one RNG call");
    test.assert(lastLower, static_cast<uint8_t>(0), "RNG lower bound");
    test.assert(lastUpper, static_cast<uint8_t>(9), "RNG inclusive upper bound");

    forcedRoll = 9;
    test.assert(Encounter::select(cache, party, forcedRng, decision), true,
                "slot nine boundary selects without an eleven-value roll");
    test.assert(decision.slot, static_cast<uint8_t>(9), "slot nine");
    test.assert(decision.creatureId, static_cast<uint8_t>(9), "slot nine ID");

    seedCache(cache, -20, 2, 8);
    forcedRoll = 0;
    test.assert(Encounter::select(cache, party, forcedRng, decision), true,
                "negative offset clamps safely");
    test.assert(decision.level, static_cast<uint8_t>(2), "lower inclusive clamp");
    seedCache(cache, 20, 2, 8);
    test.assert(Encounter::select(cache, party, forcedRng, decision), true,
                "positive offset clamps safely");
    test.assert(decision.level, static_cast<uint8_t>(8), "upper inclusive clamp");

    cache[Encounter::CACHE_VALID_OFFSET] = 0;
    rngCalls = 0;
    test.assert(Encounter::select(cache, party, forcedRng, decision), false,
                "invalid cache skips without RNG");
    test.assert(rngCalls, static_cast<uint8_t>(0), "invalid cache has no roll");

    seedCache(cache, 0, 8, 2);
    test.assert(Encounter::select(cache, party, forcedRng, decision), false,
                "reversed level bounds are malformed");
    seedCache(cache, 0, 1, 31);
    cache[4] = 32;
    forcedRoll = 4;
    test.assert(Encounter::select(cache, party, forcedRng, decision), false,
                "invalid creature ID skips safely");

    suite.addTest(test);
}

void EncounterWorldIntegrationTest(TestSuite &suite) {
    using namespace encounter_test_detail;
    Test test(__func__);
    const uint16_t tile = 0x0203;
    prepareWorld(tile, false);
    const uint32_t readsAfterLoad = fxDataFake::readCount;
    onStep(tile);
    test.assert(gameState.state, GameState_t::WORLD,
                "ordinary destination without encounter property stays in world");
    test.assert(rngCalls, static_cast<uint8_t>(0), "non-encounter tile does not roll");
    test.assert(fxDataFake::readCount, readsAfterLoad,
                "step hook performs no FX reads");

    prepareWorld(tile, true);
    const uint32_t readsBeforeStep = fxDataFake::readCount;
    forcedRoll = 9;
    onStep(tile);
    test.assert(gameState.state, GameState_t::BATTLE,
                "tagged destination enters battle");
    test.assert(legacyBattle().activeBattle, true, "battle owner is active");
    test.assert(legacyBattle().opponent.party[0].id, static_cast<uint8_t>(9),
                "battle receives slot-nine creature");
    test.assert(legacyBattle().opponent.party[0].level, static_cast<uint8_t>(5),
                "battle receives averaged level");
    const CreatureData_t encounterSeed = getCreatureFromStore(9);
    test.assert(legacyBattle().opponent.party[0].statlist.attack,
                static_cast<uint8_t>(2 * 5 + encounterSeed.atkSeed * (5 / 3)),
                "computed encounter level reaches creature stats");
    test.assert(legacyBattle().playerHealths[0], player.creatureHPs[0],
                "battle entry preserves persistent player HP");
    test.assert(fxDataFake::readCount, readsBeforeStep,
                "battle entry uses cached encounter data only");
    const uint8_t callsAfterEntry = rngCalls;
    onStep(tile);
    test.assert(rngCalls, callsAfterEntry,
                "battle state prevents duplicate encounter entry");

    suite.addTest(test);
}

void EncounterSuite(TestRunner &runner) {
    TestSuite suite("Wild encounter integration");
    EncounterDecisionTest(suite);
    EncounterWorldIntegrationTest(suite);
    runner.addTestSuite(suite);
}
