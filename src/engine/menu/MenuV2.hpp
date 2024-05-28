#pragma once
// #include "../battle/Battle.hpp"
#include "DialogMenu.hpp"
#include "../../lib/MenuStack.hpp"
#include <ArduboyFX.h>

class BattleEngine;

class MenuV2 {
  public:
    MenuEnum stack[6];
    int8_t menuPointer = -1;
    int8_t cursorIndex;
    uint8_t *moveList;
    uint8_t creatures[2];

    MenuV2();
    void setMoveList(uint8_t *pointer);
    void run(BattleEngine &engine);
    void push(MenuEnum type);
    void pop();
    void clear();
    void transverse();
    void action(BattleEngine &engine);
    void printMenu(BattleEngine &engine);
    void updateMoveList(BattleEngine &engine);
    void creatureRental();
};
