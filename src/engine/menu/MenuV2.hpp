#pragma once
// #include "../battle/Battle.hpp"
#include "DialogMenu.hpp"
#include <ArduboyFX.h>

enum MenuEnum {
    BATTLE_MOVE_SELECT,
    BATTLE_CREATURE_SELECT,
    BATTLE_OPTIONS,
    WORLD_OPTIONS,

};
class BattleEngine;

class MenuV2 {
  public:
    MenuEnum stack[6];
    int8_t menuPointer = -1;
    int8_t cursorIndex;
    uint8_t *moveList;
    DialogMenu dialogMenu;
    uint8_t creatures[2];

    MenuV2();
    void setMoveList(uint8_t *pointer);
    void run(BattleEngine &engine);
    void push(MenuEnum type);
    void pop();
    void transverse();
    void action(BattleEngine &engine);
    void printMenu(BattleEngine &engine);
    void updateMoveList(BattleEngine &engine);
    void creatureRental();
};

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