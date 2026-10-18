#pragma once
#include <stdint.h>
#include "PlantPair.hpp"
#include "PlantStage.hpp"

// The single authored first-beta plant plot. It is map tile (14,2), the plant
// GID 551 (PROP_PLANTABLE) in data/beta-area.toml; the pocket box at (9,2) is a
// separate GID 545. The plant state carries no per-tile metadata, so the world
// hook addresses this plot by its packed PlantStage index.
constexpr uint8_t FIRST_BETA_PLOT_INDEX = 0;

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