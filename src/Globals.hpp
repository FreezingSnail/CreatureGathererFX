#pragma once

#include <Arduboy2.h>
#include <ArduboyFX.h>

#include "engine/arena/Arena.hpp"
#include "engine/battle/Battle.hpp"
#include "engine/game/Gamestate.hpp"
#include "engine/game/Menu.hpp"
#include "engine/game/MenuV2.hpp"
#include "engine/world/World.hpp"
#include "fxdata/fxdata.h"
#include "player/Player.hpp"

Arduboy2 arduboy;
Player player;
GameState_t state;
Menu menu;
BattleEngine engine;
Arena arena;
WorldEngine world;
MenuV2 menu2;