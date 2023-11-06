#include "Arena.hpp"
#include "../../player/Player.hpp"
#include "../battle/Battle.hpp"
#include "../game/Menu.hpp"
#include <Arduboy2.h>
#include <ArduboyFX.h>

void Arena::arenaLoop(Arduboy2 *arduboy, Menu *menu, Player *player, BattleEngine *engine) {
    if (this->moveIndex == 12) {
        this->moveIndex = 0;
        this->registerIndex = 0;
        this->startBattle(arduboy, engine, player, menu);
    }

    if (this->registerIndex < 3) {
        menu->printMenu();
        this->registerRentals(menu, player);
        this->displayRegisteredCount(arduboy);
    } else if (this->moveIndex < 12) {
        this->registerMoves(arduboy, player);
    }
}

void Arena::registerRentals(Menu *menu, Player *player) {
    int8_t creatureID = menu->registerCreature();
    if (creatureID >= 0) {
        player->loadreature(this->registerIndex, creatureID);
        this->registerIndex++;
        if (this->registerIndex == 3) {
            this->cursor = 0;
        }
    }
}

void Arena::registerMoves(Arduboy2 *arduboy, Player *player) {

    if (this->moveIndex > 7) {
        this->moveCreature = 3;
    } else if (this->moveIndex > 3) {
        this->moveCreature = 2;
    }

    // load creature move pool
    uint8_t curMonID = player->party[this->moveCreature].id;
    uint24_t addr = MoveLists::moveList + sizeof(uint32_t) * curMonID;
    uint32_t movePool = FX::readIndexedUInt32(MoveLists::moveList, 0);
    arduboy->setCursor(0, 0);
    arduboy->print(this->moveIndex);
    addr = FX::readIndexedUInt24(CreatureData::creatureNames, curMonID);
    FX::setCursor(0, 10);
    FX::drawString(addr);
    uint8_t bitIndex = this->cursor;
    uint8_t bitGood = 0;
    this->debug = movePool;
    for (uint8_t i = 0; i < 4; i++) {
        while (bitGood == 0 && bitIndex < 32) {
            bitGood = (movePool >> (32 - bitIndex)) & 1;
            bitIndex++;
        }

        if (bitGood && bitIndex < 32) {
            FX::setCursor(10, 20 + (i * 10));
            uint24_t moveAddress = FX::readIndexedUInt24(MoveData::moveNames, bitIndex);
            FX::drawString(moveAddress);
            bitIndex++;
        }
        arduboy->setCursor(0, 20);
        arduboy->print(F(">"));
    }

    if (arduboy->justPressed(A_BUTTON)) {
        player->party[this->moveCreature].setMove(this->moveIndex % 4, this->cursor);
        this->moveIndex++;
    }
    if (arduboy->justPressed(DOWN_BUTTON)) {
        this->cursor++;
    }
    if (arduboy->justPressed(UP_BUTTON)) {
        this->cursor--;
    }
}

uint8_t Arena::selectOpponent() { return 0; }

void Arena::startBattle(Arduboy2 *arduboy, BattleEngine *engine, Player *player, Menu *menu) {
    engine->startFight(arduboy, player, menu, this->selectOpponent());
}

void Arena::displayRegisteredCount(Arduboy2 *arduboy) {
    for (uint8_t i = 0; i < this->registerIndex; i++) {
        arduboy->drawCircle(100 + (10 * i), 55, 3, WHITE);
    }
}