#pragma once
#include "../external/FlashStringHelper.h"
#include "../lib/Move.hpp"
#include "../lib/Text.hpp"

#include <ArduboyFX.h>

static void setTextColorBlack(Arduboy2 *arduboy) {
    arduboy->setTextBackground(WHITE);
    arduboy->setTextColor(BLACK);
}

static void setTextColorWhite(Arduboy2 *arduboy) {
    arduboy->setTextBackground(BLACK);
    arduboy->setTextColor(WHITE);
}

static void drawInfoRec(Arduboy2 *arduboy, uint8_t x, uint8_t y) {
    arduboy->fillRect(x - 3, y - 3, 60, 30, WHITE);
    arduboy->drawRect(x - 2, y - 2, 58, 28, BLACK);
}

static void printMoveInfo(Arduboy2 *arduboy, uint8_t index, uint8_t x, uint8_t y) {
    if (index == 32) {
        return;
    }
    drawInfoRec(arduboy, x, y);
    MoveBitSet m = getMovePack(index);
    arduboy->setCursor(x, y);

    arduboy->print(readFlashStringPointer(&typeNames[m.type]));
    arduboy->setCursor(x, y + 8);
    if (m.physical) {
        arduboy->print(F("Phys"));
    } else {
        arduboy->print(F("Spec"));
    }
    arduboy->setCursor(x, y + 16);
    arduboy->print(F("power:"));
    arduboy->print(m.power);
}

static void printBattleMenu(Arduboy2 &arduboy) {
    arduboy.setCursor(6, 46);
    arduboy.print(asFlashStringHelper(moveM));
    arduboy.setCursor(64, 46);
    arduboy.print(asFlashStringHelper(itemM));
    arduboy.setCursor(6, 54);
    arduboy.print(asFlashStringHelper(changeM));
    arduboy.setCursor(64, 54);
    arduboy.print(asFlashStringHelper(escapeM));
}

static void printCursor(Arduboy2 &arduboy, int8_t index) {
    switch (index) {
    case 0:
        arduboy.setCursor(0, 46);
        break;
    case 1:
        arduboy.setCursor(58, 46);
        break;
    case 2:
        arduboy.setCursor(0, 54);
        break;
    case 3:
        arduboy.setCursor(58, 54);
        break;
    }
    arduboy.print(F(">"));
}

static void printMoveMenu(Arduboy2 &arduboy, int8_t index, uint8_t *moveList) {

    FX::setCursor(6, 46);
    uint24_t rowAddress = FX::readIndexedUInt24(MoveData::moveNames, moveList[0]);
    FX::drawString(rowAddress);
    FX::setCursor(64, 46);
    rowAddress = FX::readIndexedUInt24(MoveData::moveNames, moveList[1]);
    FX::drawString(rowAddress);
    FX::setCursor(6, 55);
    rowAddress = FX::readIndexedUInt24(MoveData::moveNames, moveList[2]);
    FX::drawString(rowAddress);
    FX::setCursor(64, 55);
    rowAddress = FX::readIndexedUInt24(MoveData::moveNames, moveList[3]);
    FX::drawString(rowAddress);
    printMoveInfo(&arduboy, moveList[index], 38, 4);
}

static void printCreatureMenu(Arduboy2 &arduboy, uint8_t c1, uint8_t c2, Creature *cpointer) {
    cpointer->printCreature(&arduboy);
    FX::setCursor(6, 45);
    uint24_t addr = FX::readIndexedUInt24(CreatureData::creatureNames, c1);
    FX::drawString(addr);
    FX::setCursor(6, 55);
    addr = FX::readIndexedUInt24(CreatureData::creatureNames, c2);
    FX::drawString(addr);
}