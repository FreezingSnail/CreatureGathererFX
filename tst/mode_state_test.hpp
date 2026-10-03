#pragma once

#include <stddef.h>
#include <string.h>

#include "test.hpp"
#include "../src/GameState.hpp"
#include "../src/engine/ModeState.hpp"
#include "../src/engine/world/Chunk.hpp"
#include "../src/engine/world/World.hpp"
#include "../src/player/Player.hpp"

extern GameState gameState;
extern Player player;

void ModeStateLayoutTest(TestSuite &suite) {
    Test test(__func__);
    test.assert(sizeof(WorldTransient), static_cast<size_t>(191),
                "world payload is script, property window, and cache");
    test.assert(offsetof(WorldTransient, propertyWindow), static_cast<size_t>(128),
                "property window follows the 128-byte script slot");
    test.assert(offsetof(WorldTransient, zoneTableCache), static_cast<size_t>(171),
                "zone cache follows the 43-byte property window");
    test.assert(sizeof(ModeState) >= sizeof(WorldTransient), true,
                "mode union holds the complete world payload");
    suite.addTest(test);
}

void ModeTransitionPersistentStateTest(TestSuite &suite) {
    Test test(__func__);
    ModeState state;
    const GameState_t savedState = GameState_t::SAVING;
    gameState.state = savedState;
    gameState.playerLocation = 0x0A0B;
    const uint16_t savedChunk = Chunk::chunkOfLocation(gameState.playerLocation);

    player.party[0].id = 13;
    player.party[0].level = 27;
    player.party[0].moves[0] = 9;
    player.creatureHPs[0] = 81;
    memset(player.items, 0x5A, sizeof(player.items));
    uint8_t inventory[sizeof(player.items)];
    memcpy(inventory, player.items, sizeof(inventory));

    state.enterBattle();
    state.exitBattle();

    test.assert(gameState.playerLocation, static_cast<uint16_t>(0x0A0B),
                "mode transitions preserve persistent location");
    test.assert(Chunk::chunkOfLocation(gameState.playerLocation), savedChunk,
                "mode transitions preserve derived chunk identity");
    test.assert(gameState.state, savedState,
                "save state does not select or overwrite the active mode member");
    test.assert(player.party[0].id, static_cast<uint8_t>(13),
                "mode transitions preserve party creature");
    test.assert(player.party[0].level, static_cast<uint8_t>(27),
                "mode transitions preserve party level");
    test.assert(player.party[0].moves[0], static_cast<uint8_t>(9),
                "mode transitions preserve party moves");
    test.assert(player.creatureHPs[0], static_cast<uint8_t>(81),
                "mode transitions preserve the player HP owner");
    for (uint8_t i = 0; i < sizeof(inventory); ++i) {
        test.assert(reinterpret_cast<const uint8_t *>(player.items)[i], inventory[i],
                    "mode transitions preserve inventory bytes");
    }
    suite.addTest(test);
}

void ModeTransitionClearsTransientTest(TestSuite &suite) {
    Test test(__func__);
    ModeState state;
    gameState.playerLocation = 0x0607;
    for (uint8_t i = 0; i < sizeof(state.world.script); ++i) {
        state.world.script[i] = static_cast<uint8_t>(0xA5 ^ i);
    }
    memset(state.world.propertyWindow, 0xB6, sizeof(state.world.propertyWindow));
    memset(state.world.zoneTableCache, 0xC7, sizeof(state.world.zoneTableCache));

    player.creatureHPs[0] = 77;
    state.enterBattle();
    test.assert(state.battle.activeBattle, false, "fresh battle member is inactive");
    test.assert(state.battle.playerParty[0] == nullptr, true,
                "fresh battle member clears party pointers");
    test.assert(player.creatureHPs[0], static_cast<uint8_t>(77),
                "fresh battle member preserves player HP");
    test.assert(state.battle.playerAction.actionIndex, static_cast<int8_t>(-1),
                "fresh battle member resets player action");

    state.exitBattle();
    test.assert(WorldEngine::location(), static_cast<uint16_t>(0x0607),
                "world rebuild resynchronizes persistent location");
    test.assert(state.world.motion.step, static_cast<uint8_t>(0),
                "world rebuild clears movement progress");
    state.world.activateScript();
    for (uint8_t i = 0; i < sizeof(state.world.script); ++i) {
        test.assert(state.world.script[i], static_cast<uint8_t>(0),
                    "world rebuild clears script slot");
    }
    for (uint8_t i = 0; i < sizeof(state.world.propertyWindow); ++i) {
        test.assert(state.world.propertyWindow[i], static_cast<uint8_t>(0),
                    "world rebuild clears property window");
    }
    for (uint8_t i = 0; i < sizeof(state.world.zoneTableCache); ++i) {
        test.assert(state.world.zoneTableCache[i], static_cast<uint8_t>(0),
                    "world rebuild clears zone cache");
    }
    suite.addTest(test);
}

void ModeStateSuite(TestRunner &runner) {
    TestSuite suite("ModeState Suite");
    ModeStateLayoutTest(suite);
    ModeTransitionPersistentStateTest(suite);
    ModeTransitionClearsTransientTest(suite);
    runner.addTestSuite(suite);
}
