#pragma once
// #include "../battle/Battle.hpp"
#include "DialogMenu.hpp"
#include "../../lib/MenuStack.hpp"
#include "../../lib/Move.hpp"
#include <ArduboyFX.h>

class BattleEngine;

class MenuV2 {
  public:
    MenuEnum stack[6];
    int8_t menuPointer = -1;
    int8_t cursorIndex;
    uint8_t *moveList = nullptr;
    uint8_t creatures[2];
    bool drawMenu = true;
    uint24_t moveNameAddresses[4] = {};
    uint8_t moveNameIds[4] = {255, 255, 255, 255};
    uint8_t cachedNameMode = 255;
    uint24_t creatureNameAddresses[2] = {};
    uint8_t creatureNameIds[2] = {255, 255};
    uint8_t cachedRentalId = 255;
    uint24_t rentalNameAddress = 0;
    struct RentalStats {
        uint8_t type1, type2;
        uint8_t hpSeed, atkSeed, defSeed;
        uint8_t spcAtkSeed, spcDefSeed, spdSeed;
    } rentalSeed = {};

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
    void updateCreatureNames(BattleEngine &engine);
    void prepareCreatureRental();
    void creatureRental();
};
