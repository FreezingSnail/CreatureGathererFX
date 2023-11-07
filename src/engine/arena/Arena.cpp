#include "Arena.hpp"
#include "../../player/Player.hpp"
#include "../battle/Battle.hpp"
#include "../draw.h"
#include "../game/Menu.hpp"
#include <Arduboy2.h>
#include <ArduboyFX.h>

void Arena::arenaLoop(Arduboy2 &arduboy, Menu &menu, Player &player, BattleEngine &engine) {
    if (this->moveIndex == 12) {
        this->cursor = 0;
        this->movePointer = 0;
        this->moveIndex = 0;
        this->registerIndex = 0;
        this->startBattle(arduboy, engine, player, menu);
    }

    if (this->registerIndex < 3) {
        menu.printMenu();
        this->registerRentals(menu, player);
        this->displayRegisteredCount(arduboy);
    } else if (this->moveIndex < 12) {
        this->registerMoves(arduboy, player);
    }
}

void Arena::registerRentals(Menu &menu, Player &player) {
    int8_t creatureID = menu.registerCreature();
    if (creatureID >= 0) {
        player.loadreature(this->registerIndex, creatureID);
        this->registerIndex++;
        if (this->registerIndex == 3) {
            this->cursor = 0;
        }
    }
}

static void validMoves(uint32_t movePool, int8_t *moves) {
    uint8_t bitIndex = 0;
    uint8_t bitGood = 0;
    for (uint8_t i = 0; i < 16; i++) {
        moves[i] = -1;
    }
    for (uint8_t i = 0; i < 16; i++) {
        while (bitIndex < 32) {
            bitGood = (movePool >> (31 - bitIndex)) & 1;
            if (bitGood) {
                moves[i] = bitIndex;
                bitIndex++;
                break;
            }
            bitGood = 0;
            bitIndex++;
        }
    }
}

void __attribute__((optimize("-O0"))) Arena::registerMoves(Arduboy2 &arduboy, Player &player) {

    if (this->moveIndex > 7) {
        this->moveCreature = 3;
    } else if (this->moveIndex > 3) {
        this->moveCreature = 2;
    }

    // load creature move pool
    uint8_t curMonID = player.party[this->moveCreature].id;
    uint24_t addr = MoveLists::moveList + sizeof(uint32_t) * curMonID;
    uint8_t v4[4];
    FX::readDataBytes(addr, v4, sizeof(uint32_t));
    uint32_t movePool = uint32_t(v4[3]) | (uint32_t(v4[2]) << 8) | (uint32_t(v4[1]) << 16) | (uint32_t(v4[0]) << 24);
    this->debug = movePool;
    int8_t moves[16];
    validMoves(movePool, moves);
    arduboy.setCursor(0, 0);
    arduboy.print(this->moveIndex);
    addr = FX::readIndexedUInt24(CreatureData::creatureNames, curMonID);
    FX::setCursor(0, 10);
    FX::drawString(addr);
    arduboy.setCursor(10, 0);
    arduboy.print(this->cursor);
    arduboy.setCursor(20, 0);
    arduboy.print(this->movePointer);
    for (uint8_t i = 0; i < 4; i++) {
        FX::setCursor(10, 20 + (i * 10));
        int8_t move = moves[this->movePointer + i];
        if (move != -1) {
            uint24_t moveAddress = FX::readIndexedUInt24(MoveData::moveNames, move);
            FX::drawString(moveAddress);
        }
    }
    arduboy.setCursor(0, 20);
    arduboy.print(F(">"));

    if (arduboy.justPressed(A_BUTTON)) {
        this->cursor = moves[this->movePointer];
        player.party[this->moveCreature].setMove(this->cursor, this->moveIndex % 4);
        this->moveIndex++;
    }
    if (arduboy.justPressed(DOWN_BUTTON)) {
        if (this->movePointer < 16 && moves[this->movePointer + 1] != -1) {
            this->movePointer++;
            this->cursor = moves[this->movePointer];
        }
    }
    if (arduboy.justPressed(UP_BUTTON)) {
        if (this->movePointer > 0) {
            this->movePointer--;
            this->cursor = moves[this->movePointer];
        }
    }

    setTextColorBlack(&arduboy);
    printMoveInfo(&arduboy, this->cursor, 70, 10);
    setTextColorWhite(&arduboy);
}

uint8_t Arena::selectOpponent() { return 0; }

void Arena::startBattle(Arduboy2 &arduboy, BattleEngine &engine, Player &player, Menu &menu) {
    engine.startFight(arduboy, player, menu, this->selectOpponent());
}

void Arena::displayRegisteredCount(Arduboy2 &arduboy) {
    for (uint8_t i = 0; i < this->registerIndex; i++) {
        arduboy.drawCircle(100 + (10 * i), 55, 3, WHITE);
    }
}