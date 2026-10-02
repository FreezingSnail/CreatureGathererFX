#pragma once

#include <stdint.h>
#include "player/Player.hpp"
extern Player player;

#include "lib/BattleEventStack.hpp"
extern BattleEvent battleEventStack[10];

#include "lib/BattleEventPlayer.hpp"
extern BattleEventPlayer battleEventPlayer;

#include "lib/MenuStack.hpp"
extern MenuStack menuStack;

#include "GameState.hpp"
extern GameState gameState;

#include "engine/ModeState.hpp"

#include "plants/PlantGamestate.hpp"
extern PlantGameState plants;

#include "lib/ReadData.hpp"

#define XSTART 0
#define YSTART 43
#define MWIDTH 128
#define MHEIGHT 32

static PopUpDialog newDialogBox(DialogType type, uint24_t number, uint16_t damage, uint24_t animation = 0) {
    PopUpDialog dialog = {};
    dialog.height = MHEIGHT;
    dialog.width = MWIDTH;
    dialog.x = XSTART;
    dialog.y = YSTART;
    dialog.type = type;
    dialog.textAddress = 0;
    dialog.detailAddress = 0;
    dialog.damage = damage;
    dialog.animation = animation;

    switch (type) {
    case TEXT:
    case EFFECTIVENESS:
        dialog.textAddress = number;
        break;
    case NAME:
    case ENEMY_NAME:
        dialog.textAddress = readCreatureNameAddress(static_cast<uint8_t>(number));
        dialog.width = readCreatureNameWidth(static_cast<uint8_t>(number));
        dialog.height = 0;
        if (damage != 0) {
            dialog.detailAddress = readMoveNameAddress(damage);
            dialog.height = readMoveNameWidth(damage);
        }
        break;
    case FAINT:
    case SWITCH:
    case LOSS:
        dialog.textAddress = readCreatureNameAddress(static_cast<uint8_t>(number));
        dialog.width = readCreatureNameWidth(static_cast<uint8_t>(number));
        break;
    case PLAYER_EFFECT:
    case ENEMY_EFFECT:
        dialog.textAddress = readCreatureNameAddress(static_cast<uint8_t>(number));
        dialog.width = readCreatureNameWidth(static_cast<uint8_t>(number));
        dialog.detailAddress = readEffectStringAddress();
        dialog.height = readEffectStringWidth();
        break;
    default:
        break;
    }

    return dialog;
}

#include "engine/menu/DialogMenu.hpp"
extern DialogMenu dialogMenu;

#include "vm/ScriptVM.hpp"
extern ScriptVm vm;

extern uint8_t *buffer;

#ifdef TEST
extern uint8_t sBuffer[1024];
#endif
