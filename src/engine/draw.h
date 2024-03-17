#pragma once
#include "../creature/Creature.hpp"
#include "../lib/Move.hpp"
#include "../external/Font4x6.h"
#include "../common.hpp"

#include <ArduboyFX.h>
static void printType(Type t, uint8_t x, uint8_t y) {
    switch (t) {
    case Type::SPIRIT:
        printString(font, MenuFXData::SPIRIT, x, y);
        break;
    case Type::WATER:
        printString(font, MenuFXData::WATER, x, y);
        break;
    case Type::WIND:
        printString(font, MenuFXData::WIND, x, y);
        break;
    case Type::EARTH:
        printString(font, MenuFXData::EARTH, x, y);
        break;
    case Type::FIRE:
        printString(font, MenuFXData::FIRE, x, y);
        break;
    case Type::LIGHTNING:
        printString(font, MenuFXData::LIGHTNING, x, y);
        break;
    case Type::PLANT:
        printString(font, MenuFXData::PLANT, x, y);
        break;
    case Type::ELDER:
        printString(font, MenuFXData::EARTH, x, y);
        break;
    }
}

static void setTextColorBlack() {
    FX::setFontMode(dcfWhiteBlack);
    font.setTextColor(BLACK);
}

static void setTextColorWhite() {
    FX::setFontMode(dcmWhite);
    font.setTextColor(WHITE);
}

static void drawInfoRec(uint8_t x, uint8_t y) {
    // Arduboy2::fillRect(x - 3, y - 3, 60, 30, WHITE);
    // Arduboy2::drawRect(x - 2, y - 2, 58, 28, BLACK);
    FX::drawBitmap(x - 3, y - 3, moveInfo, 0, dbmNormal);
}

static void printMoveInfo(uint8_t index, uint8_t x, uint8_t y) {
    if (index == 32) {
        return;
    }
    setTextColorBlack();
    drawInfoRec(x, y);
    MoveBitSet m = getMovePack(index);

    printType(Type(m.type), x, y);
    if (m.physical) {
        printString(font, MenuFXData::physical, x, y + 8);
    } else {
        printString(font, MenuFXData::special, x, y + 8);
    }
    printString(font, MenuFXData::power, x, y + 16);
    font.setCursor(x + 30, y + 16);
    font.print(m.power);
}

static void printBattleMenu(int8_t index) {
}

static void printCursor(int8_t index) {
    switch (index) {
    case 0:
        FX::setCursor(0, 46);
        break;
    case 1:
        FX::setCursor(58, 46);
        break;
    case 2:
        FX::setCursor(0, 54);
        break;
    case 3:
        FX::setCursor(58, 54);
        break;
    }
    // FX::setFont(arduboyFont, dbmReverse);
    //  FX::drawChar('*');
    // FX::setFont(font4x6, dbmNormal);
}

static void printMoveMenu(int8_t index, uint8_t *moveList) {

    uint24_t rowAddress = FX::readIndexedUInt24(MoveData::moveNames, moveList[0]);
    printString(font, rowAddress, 6, 45);
    rowAddress = FX::readIndexedUInt24(MoveData::moveNames, moveList[1]);
    printString(font, rowAddress, 69, 45);
    rowAddress = FX::readIndexedUInt24(MoveData::moveNames, moveList[2]);
    printString(font, rowAddress, 6, 53);
    rowAddress = FX::readIndexedUInt24(MoveData::moveNames, moveList[3]);
    printString(font, rowAddress, 69, 53);
    printMoveInfo(moveList[index], 38, 4);
}

static void printCreatureMenu(uint8_t c1, uint8_t c2, Creature *cpointer, uint8_t index) {
    SpritesU::fillRect(0, 33, 128, 31, WHITE);
    SpritesU::fillRect(0, 0, 128, 32, BLACK);
    cpointer->printCreature();
    uint24_t c1addr = FX::readIndexedUInt24(CreatureNames::CreatureNames, c1);
    uint24_t c2addr = FX::readIndexedUInt24(CreatureNames::CreatureNames, c2);
    if (index == 0) {
        SpritesU::drawOverwriteFX(6, 49, c1addr, FRAME(0));
        SpritesU::drawOverwriteFX(6, 56, c2addr, FRAME(1));
    } else {
        SpritesU::drawOverwriteFX(6, 49, c1addr, FRAME(1));
        SpritesU::drawOverwriteFX(6, 56, c2addr, FRAME(0));
    }
}