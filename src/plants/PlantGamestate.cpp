#include "PlantGamestate.hpp"

void PlantGameState::tick() {
    ticker++;
    if (ticker == GROWTH_STEPS) {
        ticker = 0;
        plantStages.incrementAll();
    }
}

void PlantGameState::save() {
}

void PlantGameState::load(uint8_t mapIndex) {
}

void PlantGameState::changeArea(uint8_t mapIndex) {
}
