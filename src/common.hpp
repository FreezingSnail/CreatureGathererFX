#pragma once

#include <stdint.h>

#define ABG_TIMER1
#define ABG_SYNC_PARK_ROW

#include "external/ArduboyG.h"
extern ArduboyGBase_Config<ABG_Mode::L4_Triplane> arduboy;

#define SPRITESU_OVERWRITE
#define SPRITESU_PLUSMASK
#define SPRITESU_RECT
#define SPRITESU_FX
#include "external/SpritesU.hpp"

#include "Animator.hpp"
extern Animator animator;

#include "engine/menu/MenuV2.hpp"
extern MenuV2 menu;
#include "GameState.hpp"
extern GameState gameState;
#include "engine/battle/Battle.hpp"
extern BattleEngine engine;
#include "player/Player.hpp"
extern Player player;

#define FRAME(x) x * 3 + arduboy.currentPlane()

#define DGF __attribute__((optimize("-O0")))

#include "fxdata.h"
static void drawStatNumbers(uint8_t x, uint8_t y, uint8_t number) {
    uint8_t upper = number / 100;
    uint8_t lower = number % 100;
    SpritesU::drawPlusMaskFX(x, y, singlenumberswhite, FRAME(upper));
    SpritesU::drawPlusMaskFX(x + 4, y, numberswhite, FRAME(lower));
}

static void drawNumbersBlack(uint8_t x, uint8_t y, uint8_t number) {
    uint8_t upper = number / 100;
    uint8_t lower = number % 100;
    SpritesU::drawPlusMaskFX(x, y, singlenumbersblack, FRAME(upper));
    SpritesU::drawPlusMaskFX(x + 4, y, numbersblack, FRAME(lower));
}

#include "lib/BattleEventStack.hpp"
extern BattleEvent battleEventStack[10];

#include "lib/BattleEventPlayer.hpp"
extern BattleEventPlayer battleEventPlayer;

#include "lib/MenuStack.hpp"
extern MenuStack menuStack;

static PopUpDialog newDialogBox(DialogType type, uint24_t number, uint16_t damage, uint24_t animation = 0) {
    PopUpDialog dialog;
    dialog.height = MHEIGHT;
    dialog.width = MWIDTH;
    dialog.x = XSTART;
    dialog.y = YSTART;
    dialog.type = type;
    dialog.textAddress = number;
    dialog.damage = damage;
    dialog.animation = animation;

    return dialog;
}

#include "engine/menu/DialogMenu.hpp"
extern DialogMenu dialogMenu;