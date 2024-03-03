#pragma once
#include <stdint.h>
#include "PlantPair.hpp"
#include "PlantStage.hpp"

class PlantGameState {
  public:
    PlantStage plantStages;
    PlantPair plantPairs[2];
    uint8_t ticker = 0;

    void tick();
    void save();
    void load(uint8_t mapIndex);
    void changeArea(uint8_t mapIndex);
};