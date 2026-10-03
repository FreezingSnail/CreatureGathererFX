#include "StepEvent.hpp"

#include "../../plants/PlantGamestate.hpp"

extern PlantGameState plants;

namespace {
void decayLureOnStep(uint16_t tile) {
    (void)tile;
}

void rollEncounterOnStep(uint16_t tile) {
    (void)tile;
}
}

void onStep(uint16_t tile) {
    plants.tick();
    decayLureOnStep(tile);
    rollEncounterOnStep(tile);
}
