#pragma once

#include "player/Player.hpp"
extern Player player;

#include "lib/BattleEventStack.hpp"
extern BattleEvent battleEventStack[10];

#include "lib/BattleEventPlayer.hpp"
extern BattleEventPlayer battleEventPlayer;

#include "lib/MenuStack.hpp"
extern MenuStack menuStack;

#define XSTART 0
#define YSTART 43
#define MWIDTH 128
#define MHEIGHT 32

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