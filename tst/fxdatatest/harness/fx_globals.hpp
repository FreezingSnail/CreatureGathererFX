#pragma once

#define ABG_IMPLEMENTATION
#define SPRITESU_IMPLEMENTATION
#include "../src/common.hpp"

#ifdef FX_GLOBALS_MINIMAL
#include "../src/engine/menu/DialogMenu.hpp"

decltype(arduboy) arduboy;
DialogMenu dialogMenu;
#else
#include "../src/globals.hpp"

#include "../src/engine/arena/Arena.hpp"
#include "../src/engine/battle/Battle.hpp"
#include "../src/engine/game/Gamestate.hpp"
#include "../src/engine/menu/MenuV2.hpp"
#include "../src/engine/world/Event.hpp"
#include "../src/engine/world/World.hpp"
#include "../src/fxdata.h"
#include "../src/player/Player.hpp"
#include "../src/plants/PlantGamestate.hpp"

decltype(arduboy) arduboy;

uint8_t debug;
GameState gameState;
MenuV2 menu = MenuV2();
BattleEngine engine;
Player player = Player();
Arena arena = Arena();
WorldEngine world;
Animator animator = Animator();
PlantGameState plants;

BattleEvent battleEventStack[10];
BattleEventPlayer battleEventPlayer;
MenuStack menuStack;
DialogMenu dialogMenu;
#endif

inline void fxTestSetup(uint24_t firstPage = 0, uint24_t lastPage = 32767) {
    Serial.begin(9600);
    arduboy.begin();
    FX::begin(FX_DATA_PAGE, FX_SAVE_PAGE);
    FX::setCursorRange(firstPage, lastPage);
}
