#pragma once

#include <string.h>

#include "fxtest.hpp"
#include "src/engine/ModeState.hpp"
#include "src/engine/battle/BattleState.hpp"
#include "src/engine/battle/ActionResult.hpp"
#include "src/engine/world/Chunk.hpp"
#include "src/engine/world/Encounter.hpp"
#include "src/engine/world/World.hpp"
#include "src/fxdata.h"
#include "src/player/Player.hpp"

inline void test_mode_state(FxTest &test)
{
    test.expectEq(sizeof(WorldTransient), 193, F("world payload size"));
    test.expectEq(sizeof(ModeState), sizeof(WorldTransient), F("AVR mode union size"));
    test.expectEq(alignof(WorldTransient), 1, F("world payload alignment"));
    test.expectEq(sizeof(battle::Combatant), 6, F("AVR duel combatant size"));
    test.expectEq(sizeof(battle::BattleState), 46, F("AVR duel battle state size"));
#ifdef CGFX_BATTLE_COMPACT_RESULT
    test.expectEq(sizeof(battle::ActionResult), 7, F("compact action result size"));
    test.expectEq(sizeof(battle::BattleSession), 142, F("compact battle session size"));
#else
    test.expectEq(sizeof(battle::ActionResult), 28, F("AVR action result size"));
#endif

    gameState.playerLocation = 0x0A0B;
    player.party[0].id = 13;
    player.creatureHPs[0] = 81;
    memset(&player.inventory, 0x5A, sizeof(player.inventory));

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
    for (uint8_t i = 0; i < sizeof(player.inventory); ++i) {
        test.expectEq(reinterpret_cast<const uint8_t *>(&player.inventory)[i], 0x5A,
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
    // exitBattle() value-initialises the world, so the battle round trip is a
    // world re-entry: the chunk-change transition loads the authored zone for
    // the player's chunk. Read the packed zone independently and prove the
    // cache holds exactly that payload after the transition.
    const uint16_t chunk = Chunk::chunkOfLocation(0x0A0B);
    uint8_t zone[Encounter::ZONE_BYTES] = {};
    bool zoneFound = false;
    for (uint8_t index = 0; index < Encounter::ZONE_COUNT; ++index) {
        FX::readDataBytes(EncounterFXData::zoneDefs +
                              static_cast<uint24_t>(index) * Encounter::ZONE_BYTES,
                          zone, sizeof(zone));
        if ((static_cast<uint16_t>(zone[0]) |
             (static_cast<uint16_t>(zone[1]) << 8)) == chunk) {
            zoneFound = true;
            break;
        }
    }
    test.expectEq(zoneFound, true, F("authored zone for the player chunk"));
    uint8_t packedTable[Encounter::TABLE_BYTES];
    FX::readDataBytes(EncounterFXData::tables +
                          static_cast<uint24_t>(zone[2]) * Encounter::TABLE_BYTES,
                      packedTable, sizeof(packedTable));
    for (uint8_t i = 0; i < Encounter::TABLE_BYTES; ++i) {
        test.expectEq(worldState().zoneTableCache[i], packedTable[i],
                      F("cache holds the packed base table"));
    }
    test.expectEq(worldState().zoneTableCache[Encounter::CACHE_VALID_OFFSET], 1,
                  F("zone cache valid after world re-entry"));
    test.expectEq(worldState().zoneTableCache[Encounter::CACHE_CHUNK_LOW_OFFSET], zone[0],
                  F("cache chunk low matches the packed zone"));
    test.expectEq(worldState().zoneTableCache[Encounter::CACHE_TABLE_COUNT_OFFSET], zone[3],
                  F("cache table count matches the packed zone"));
    test.expectEq(worldState().zoneTableCache[Encounter::CACHE_TYPE_MASK_OFFSET], zone[4],
                  F("cache type mask matches the packed zone"));
    test.expectEq(worldState().zoneTableCache[Encounter::CACHE_LEVEL_OFFSET], zone[9],
                  F("cache level offset matches the packed zone"));
    test.expectEq(worldState().zoneTableCache[Encounter::CACHE_NIBBLES_OFFSET],
                  static_cast<uint8_t>((zone[5] & 0x0F) | ((zone[6] & 0x0F) << 4)),
                  F("cache lure nibbles match the packed zone"));
    test.expectEq(worldState().zoneTableCache[Encounter::CACHE_NIBBLES_OFFSET + 1],
                  static_cast<uint8_t>((zone[7] & 0x0F) | ((zone[8] & 0x0F) << 4)),
                  F("cache lure nibbles match the packed zone"));

    // Authored coverage ends at the area bounds: crossing into a chunk with no
    // authored zone still invalidates the cache (Encounter::load failure).
    WorldEngine::setPos(worldState(), 0, 0);
    for (uint8_t i = 0; i < sizeof(worldState().zoneTableCache); ++i) {
        test.expectEq(worldState().zoneTableCache[i], 0, F("zoneless chunk clears the cache"));
    }
}
