#pragma once

#include <stdint.h>
#include "player/Player.hpp"
extern Player player;

#include "GameState.hpp"
extern GameState gameState;

#include "engine/ModeState.hpp"

#include "plants/PlantGamestate.hpp"
extern PlantGameState plants;

#include "engine/menu/DialogMenu.hpp"
extern DialogMenu dialogMenu;

// Compact world scripts support Msg for authored NPC dialog, so the queue is
// part of the ordinary shipping profile. Full VM builds keep the same queue;
// explicit demos and test builds also need popup presentation.
#if defined(CGFX_FULL_WORLD_VM) || defined(CGFX_COMPACT_WARP_VM) || defined(TEST) || \
    defined(FX_READ_COUNTER) || defined(CGFX_TRAINER_DEMO) || \
    defined(CGFX_ARENA_DEMO) || defined(CGFX_WILD_DEMO)
#define CGFX_DIALOG_QUEUE_ENABLED 1
#else
#define CGFX_DIALOG_QUEUE_ENABLED 0
#endif

#include "engine/menu/MenuV2.hpp"
extern MenuV2 menu;

#include "vm/ScriptVM.hpp"
extern ScriptVm vm;

extern uint8_t *buffer;

#ifdef TEST
extern uint8_t sBuffer[1024];
#endif
