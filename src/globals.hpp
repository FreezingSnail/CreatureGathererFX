#pragma once

#include <stdint.h>
#include "player/Player.hpp"
extern Player player;

#include "lib/MenuStack.hpp"
extern MenuStack menuStack;

#include "GameState.hpp"
extern GameState gameState;

#include "engine/ModeState.hpp"

#include "plants/PlantGamestate.hpp"
extern PlantGameState plants;

#include "engine/menu/DialogMenu.hpp"
extern DialogMenu dialogMenu;

#include "engine/menu/MenuV2.hpp"
extern MenuV2 menu;

#include "vm/ScriptVM.hpp"
extern ScriptVm vm;

extern uint8_t *buffer;

#ifdef TEST
extern uint8_t sBuffer[1024];
#endif
