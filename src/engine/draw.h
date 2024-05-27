#pragma once
#include "../creature/Creature.hpp"
#include "../lib/Move.hpp"
// #include "../external/Font4x6.h"
#include "../common.hpp"
#include "../lib/ReadData.hpp"

#include <ArduboyFX.h>

static void printType(Type t, uint8_t x, uint8_t y) {
    switch (t) {
    case Type::SPIRIT:
        SpritesU::drawOverwriteFX(x, y, spirit, FRAME(0));
        break;
    case Type::WATER:
        SpritesU::drawOverwriteFX(x, y, water, FRAME(0));
        break;
    case Type::WIND:
        SpritesU::drawOverwriteFX(x, y, wind, FRAME(0));
        break;
    case Type::EARTH:
        SpritesU::drawOverwriteFX(x, y, earth, FRAME(0));
        break;
    case Type::FIRE:
        SpritesU::drawOverwriteFX(x, y, fire, FRAME(0));
        break;
    case Type::LIGHTNING:
        SpritesU::drawOverwriteFX(x, y, lightning, FRAME(0));
        break;
    case Type::PLANT:
        SpritesU::drawOverwriteFX(x, y, plant, FRAME(0));
        break;
    case Type::ELDER:
        SpritesU::drawOverwriteFX(x, y, elder, FRAME(0));
        break;
    }
}

static void setTextColorBlack() {
    FX::setFontMode(dcfWhiteBlack);
    // font.setTextColor(BLACK);
}

static void setTextColorWhite() {
    FX::setFontMode(dcmWhite);
    // font.setTextColor(WHITE);
}

static void drawInfoRec(uint8_t x, uint8_t y) {
    SpritesU::fillRect(x - 3, y - 3, 60, 30, BLACK);
    // Arduboy2::drawRect(x - 2, y - 2, 58, 28, BLACK);
    FX::drawBitmap(x - 3, y - 3, moveInfo, 0, dbmNormal);
}

static void DGF printMoveInfo(uint8_t index, uint8_t x, uint8_t y) {
    if (index == 32) {
        return;
    }
    setTextColorBlack();
    drawInfoRec(x, y);
    MoveBitSet m = getMovePack(index);

    printType(Type(m.type), x, y);
    if (m.physical) {
        SpritesU::drawOverwriteFX(x, y + 8, physical, FRAME(0));
        // printString(font, MenuFXData::physical, x, y + 8);
    } else {
        SpritesU::drawOverwriteFX(x, y + 8, special, FRAME(0));
        // printString(font, MenuFXData::special, x, y + 8);
    }
    SpritesU::drawOverwriteFX(x, y + 16, power, FRAME(0));
    // printString(font, MenuFXData::power, x, y + 16);
    // font.setCursor(x + 30, y + 16);
    // font.print(m.power);
    drawStatNumbers(x + 33, y + 17, m.power);
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

    uint8_t color[4] = {1, 1, 1, 1};
    if (moveList[index] != 32) {
        color[index] = 0;
    }
    uint24_t rowAddress = FX::readIndexedUInt24(MoveNames::MoveNames, moveList[0]);
    SpritesU::drawOverwriteFX(6, 45, rowAddress, FRAME(color[0]));
    // printString(font, rowAddress, 6, 45);
    rowAddress = FX::readIndexedUInt24(MoveNames::MoveNames, moveList[1]);
    SpritesU::drawOverwriteFX(69, 45, rowAddress, FRAME(color[1]));
    // printString(font, rowAddress, 69, 45);
    rowAddress = FX::readIndexedUInt24(MoveNames::MoveNames, moveList[2]);
    SpritesU::drawOverwriteFX(6, 53, rowAddress, FRAME(color[2]));
    // printString(font, rowAddress, 6, 53);
    rowAddress = FX::readIndexedUInt24(MoveNames::MoveNames, moveList[3]);
    SpritesU::drawOverwriteFX(69, 53, rowAddress, FRAME(color[3]));
    // printString(font, rowAddress, 69, 53);
    printMoveInfo(moveList[index], 38, 4);
}

// TODO deal with drawing numbers
static void printCreature(Creature *creature) {
    SpritesU::drawOverwriteFX(35, 0, hpText, FRAME(0));
    SpritesU::drawOverwriteFX(35, 10, atkText, FRAME(0));
    SpritesU::drawOverwriteFX(35, 20, defText, FRAME(0));
    SpritesU::drawOverwriteFX(72, 0, satkText, FRAME(0));
    SpritesU::drawOverwriteFX(72, 10, sdefText, FRAME(0));
    SpritesU::drawOverwriteFX(72, 20, spdText, FRAME(0));

    drawStatNumbers(60, 0, creature->statlist.hp);
    drawStatNumbers(60, 10, creature->statlist.attack);
    drawStatNumbers(60, 20, creature->statlist.defense);
    drawStatNumbers(103, 0, creature->statlist.spcAtk);
    drawStatNumbers(103, 10, creature->statlist.spcDef);
    drawStatNumbers(103, 20, creature->statlist.speed);

    for (uint8_t i = 0; i < 4; i++) {
        if (creature->moves[i] == 32) {
            continue;
        }
        uint8_t offset = i % 2;
        uint8_t x = 4 + (60 * offset);
        uint8_t y = 34 + (8 * (i / 2));

        uint24_t rowAddress = FX::readIndexedUInt24(MoveNames::MoveNames, creature->moves[i]);
        SpritesU::drawOverwriteFX(x, y, rowAddress, FRAME(1));
    }
}

static void printCreatureMenu(uint8_t c1, uint8_t c2, Creature *cpointer, uint8_t index) {
    SpritesU::fillRect(0, 33, 128, 31, WHITE);
    SpritesU::fillRect(0, 0, 128, 32, BLACK);
    printCreature(cpointer);
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
