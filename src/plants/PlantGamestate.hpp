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

// Frames within the appended pocketProps image: the authored plot (GID 551)
// uses its RAM stage (0 empty .. 3 mature) and the deposit box (GID 545) uses
// the appended lure-box frame. Any other tile keeps the shared tile sheet.
constexpr uint8_t POCKET_PROPS_NONE = 0xFF;
constexpr uint8_t pocketPropsFrame(uint16_t tileId, uint8_t plotStage) {
    return tileId == 551 ? plotStage
                         : (tileId == 545 ? 4 : POCKET_PROPS_NONE);
}