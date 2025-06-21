#pragma once
#include <stdint.h>
#include "engine/game/Gamestate.hpp"

// #include "engine/arena/Arena.hpp"
// #include "engine/battle/Battle.hpp"
// #include "engine/menu/MenuV2.hpp"
// #include "engine/world/Event.hpp"
// #include "engine/world/World.hpp"
// #include "fxdata.h"
// #include "player/Player.hpp"
// #include "Animator.hpp"
// #include "plants/PlantGamestate.hpp"
// #include "lib/uint24.h"

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

    GameState();
};
