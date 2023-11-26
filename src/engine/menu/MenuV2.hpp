#pragma once
#include "../battle/Battle.hpp"
#include "DialogMenu.hpp"
#include <ArduboyFX.h>

enum MenuEnum {
    BATTLE_MOVE_SELECT,
    BATTLE_CREATURE_SELECT,
    BATTLE_OPTIONS,
    WORLD_OPTIONS,
};

class MenuV2 {
  public:
    MenuEnum stack[4];
    int8_t menuPointer = -1;
    int8_t cursorIndex;
    uint8_t *moveList;
    DialogMenu dialogMenu;
    uint8_t creatures[2];

    void setMoveList(uint8_t *pointer);
    void run(Arduboy2 &arduboy, BattleEngine &engine);
    void push(MenuEnum type);
    void pop();
    void transverse(Arduboy2 &arduboy);
    void action(Arduboy2 &arduboy, BattleEngine &engine);
    void print(Arduboy2 &arduboy, BattleEngine &engine);
    void updateMoveList(BattleEngine &engine);
};