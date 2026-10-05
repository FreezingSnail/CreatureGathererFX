#pragma once

#include <string.h>

#include "fxtest.hpp"
#include "src/engine/ModeState.hpp"
#include "src/engine/battle/BattleState.hpp"
#include "src/engine/battle/ActionResult.hpp"
#include "src/engine/world/Chunk.hpp"
#include "src/engine/world/World.hpp"
#include "src/player/Player.hpp"

inline void test_mode_state(FxTest &test)
{
    test.expectEq(sizeof(WorldTransient), 191, F("world payload size"));
    test.expectEq(sizeof(ModeState), sizeof(WorldTransient), F("AVR mode union size"));
    test.expectEq(alignof(WorldTransient), 1, F("world payload alignment"));
    test.expectEq(sizeof(battle::Combatant), 36, F("AVR combatant size"));
    test.expectEq(sizeof(battle::BattleState), 124, F("AVR battle state size"));
    test.expectEq(sizeof(battle::ActionResult), 28, F("AVR action result size"));

    gameState.playerLocation = 0x0A0B;
    player.party[0].id = 13;
    player.creatureHPs[0] = 81;
    memset(player.items, 0x5A, sizeof(player.items));

    for (uint8_t i = 0; i < sizeof(worldState().script); ++i) {
        worldState().script[i] = static_cast<uint8_t>(0xA5 ^ i);
    }
    memset(worldState().propertyWindow, 0xB6, sizeof(worldState().propertyWindow));
    memset(worldState().zoneTableCache, 0xC7, sizeof(worldState().zoneTableCache));

    enterBattle();
    test.expectEq(battleSession().isActive(), false, F("fresh battle session is inactive"));
    test.expectEq(battleSession().awaitingPlayer(), false,
                  F("fresh battle session has no pending choice"));
    test.expectEq(player.creatureHPs[0], 81, F("battle entry preserves player HP"));

    exitBattle();
    test.expectEq(gameState.playerLocation, 0x0A0B, F("persistent location retained"));
    test.expectEq(Chunk::chunkOfLocation(gameState.playerLocation),
                  Chunk::chunkOfLocation(0x0A0B), F("persistent chunk retained"));
    test.expectEq(player.party[0].id, 13, F("party retained"));
    test.expectEq(player.creatureHPs[0], 81, F("player HP retained"));
    for (uint8_t i = 0; i < sizeof(player.items); ++i) {
        test.expectEq(reinterpret_cast<const uint8_t *>(player.items)[i], 0x5A,
                      F("inventory retained"));
    }
    test.expectEq(WorldEngine::location(), 0x0A0B, F("world resynced from location"));

    worldState().activateScript();
    for (uint8_t i = 0; i < sizeof(worldState().script); ++i) {
        test.expectEq(worldState().script[i], 0, F("script slot cleared"));
    }
    for (uint8_t i = 0; i < sizeof(worldState().propertyWindow); ++i) {
        test.expectEq(worldState().propertyWindow[i], 0, F("property window cleared"));
    }
    for (uint8_t i = 0; i < sizeof(worldState().zoneTableCache); ++i) {
        test.expectEq(worldState().zoneTableCache[i], 0, F("zone cache cleared"));
    }
}
