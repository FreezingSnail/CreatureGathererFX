#include "StepEvent.hpp"

#include "TilePropertyWindow.hpp"
#include "../ModeState.hpp"
#include "../../GameState.hpp"
#include "../../engine/game/Gamestate.hpp"
#include "../../player/Player.hpp"
#include "../../plants/PlantGamestate.hpp"

extern GameState gameState;
extern Player player;
extern PlantGameState plants;

namespace {
void decayLureOnStep(uint16_t tile) {
    (void)tile;
}

void rollEncounterOnStep(uint16_t tile) {
#ifdef WORLD_TEST
    (void)tile;
    return;
#else
    if (gameState.state != GameState_t::WORLD) return;

    WorldTransient &world = worldState();
    TilePropertyWindow properties(world.propertyWindow);
    uint8_t tileProperties = 0;
    bool occupied = false;
    const int16_t x = static_cast<uint8_t>(tile);
    const int16_t y = static_cast<uint8_t>(tile >> 8);
    if (!properties.lookup(x, y, tileProperties, occupied) || !occupied ||
        (tileProperties & TileProps::PROP_ENCOUNTER) == 0) {
        return;
    }

    Encounter::Decision decision = {};
    if (!Encounter::select(world.zoneTableCache, player.party, nullptr,
                           decision)) {
        return;
    }

    // The world member is destroyed by enterBattle(). Do not touch `world`
    // or any other WorldTransient field after this call.
    enterBattle();
    legacyBattle().startEncounter(decision.creatureId, decision.level);
    gameState.state = GameState_t::BATTLE;
#endif
}
}

void onStep(uint16_t tile) {
    plants.tick();
    decayLureOnStep(tile);
    rollEncounterOnStep(tile);
}

#ifdef TEST
namespace StepEventTest {
void setEncounterRng(Encounter::Rng rng) {
    Encounter::setRngForTest(rng);
}
}
#endif
