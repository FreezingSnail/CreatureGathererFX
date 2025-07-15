#pragma once
#include <stdint.h>
#include "engine/game/Gamestate.hpp"

class GameState {
  public:
    // PlantGameState plants;
    // Player player;
    GameState_t state;
    // BattleEngine engine;
    // Arena arena;
    // WorldEngine world;
    // Animator animator;
    // tile index on 1d flattened map
    uint16_t playerLocation;
    uint8_t *flags;
    uint8_t debug;
    uint8_t gameControlFlags;
    // TODO: these can be condesed to 1 byte since its 0-16
    int8_t xStepOffset = 0;
    int8_t yStepOffset = 0;
    // TODO: only needs 4 bits
    uint8_t walkingMask = 0;

    GameState();
    void setFlag(uint16_t index);
    bool getFlag(uint16_t index);
    void clearFlag(uint16_t index);
};
