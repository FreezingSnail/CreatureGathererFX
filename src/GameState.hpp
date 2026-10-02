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
    GameState();
    void setFlag(uint16_t index);
    bool getFlag(uint16_t index);
    void clearFlag(uint16_t index);
};
