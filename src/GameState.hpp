#pragma once

#include "src/engine/arena/Arena.hpp"
#include "src/engine/battle/Battle.hpp"
#include "src/engine/game/Gamestate.hpp"
#include "src/engine/menu/MenuV2.hpp"
#include "src/engine/world/Event.hpp"
#include "src/engine/world/World.hpp"
#include "src/fxdata.h"
#include "src/player/Player.hpp"
#include "src/Animator.hpp"
#include "src/plants/PlantGamestate.hpp"

class GameState {
  public:
    PlantGameState plants;
    Player player;
    GameState_t state;
    BattleEngine engine;
    Arena arena;
    WorldEngine world;
    MenuV2 menu2;
    Font4x6 font;
    Animator animator;

    GameState();
};