#include "StepEvent.hpp"

#include "TilePropertyWindow.hpp"
#include "LureZone.hpp"
#include "../ModeState.hpp"
#include "../../GameState.hpp"
#include "../../engine/game/Gamestate.hpp"
#include "../../player/Player.hpp"
#include "../../plants/PlantGamestate.hpp"
#include "../../lib/random.hpp"

extern GameState gameState;
extern Player player;
extern PlantGameState plants;

namespace {
#ifdef CGFX_WILD_DEMO
bool wildDemoEncounterPending = true;
#endif

// FX_READ_COUNTER marks the instrumented device-test builds, which need the
// deterministic roll seam just like the host TEST build; shipping has neither.
#if defined(TEST) || defined(FX_READ_COUNTER)
Encounter::Rng testRng = nullptr;
#endif

// One in ten encounter-prop steps rolls a wild encounter.
constexpr uint8_t ENCOUNTER_STEP_PERCENT = 10;

uint8_t rollRange(uint8_t lower, uint8_t upper) {
#if defined(TEST) || defined(FX_READ_COUNTER)
    if (testRng != nullptr) return testRng(lower, upper);
#endif
    return randomRoll(lower, upper);
}

Encounter::Rng encounterRng() {
#if defined(TEST) || defined(FX_READ_COUNTER)
    return testRng;
#else
    return nullptr;
#endif
}

void decayLureOnStep(uint16_t tile) {
    (void)tile;
    LureZone::onStep();
}

void rollEncounterOnStep(uint16_t tile) {
#ifdef WORLD_TEST
    (void)tile;
    return;
#else
    if (gameState.state != GameState_t::WORLD) return;

    WorldTransient &world = worldState();
#ifdef CGFX_WILD_DEMO
    // Production map data currently has no encounter tiles. The opt-in demo
    // substitutes one first-step trigger while preserving normal world
    // movement, encounter selection, and BattleSession setup.
    if (wildDemoEncounterPending) {
        wildDemoEncounterPending = false;
    } else {
        return;
    }
#else
    TilePropertyWindow properties(world.propertyWindow);
    uint8_t tileProperties = 0;
    bool occupied = false;
    const int16_t x = static_cast<uint8_t>(tile);
    const int16_t y = static_cast<uint8_t>(tile >> 8);
    if (!properties.lookup(x, y, tileProperties, occupied) || !occupied ||
        (tileProperties & TileProps::PROP_ENCOUNTER) == 0) {
        return;
    }
    // Gate the roll before any table selection so most encounter-prop steps
    // stay free of encounter work.
    if (rollRange(0, 99) >= ENCOUNTER_STEP_PERCENT) return;
#endif

    Encounter::Decision decision = {};
    if (!Encounter::select(world.zoneTableCache, player.party, encounterRng(),
                           LureZone::tableOffset(), decision)) {
        return;
    }

    // The world member is destroyed by enterBattle(). Do not touch `world`
    // or any other WorldTransient field after this call.
    enterBattle();
    battleSession().beginWild(decision.creatureId, decision.level, false, 0);
    gameState.state = GameState_t::BATTLE;
#endif
}
}

void onStep(uint16_t tile) {
    plants.tick();
    decayLureOnStep(tile);
    rollEncounterOnStep(tile);
}

#if defined(TEST) || defined(FX_READ_COUNTER)
namespace StepEventTest {
void setEncounterRng(Encounter::Rng rng) {
    testRng = rng;
}
}
#endif
