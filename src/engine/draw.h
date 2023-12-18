#pragma once
#include "../creature/Creature.hpp"
#include "../external/FlashStringHelper.h"
#include "../lib/Move.hpp"

#include <ArduboyFX.h>
static void printType(Type t) {
    switch (t) {
    case Type::SPIRIT:
        FX::drawString(MenuFXData::SPIRIT);
        break;
    case Type::WATER:
        FX::drawString(MenuFXData::WATER);
        break;
    case Type::WIND:
        FX::drawString(MenuFXData::WIND);
        break;
    case Type::EARTH:
        FX::drawString(MenuFXData::EARTH);
        break;
    case Type::FIRE:
        FX::drawString(MenuFXData::FIRE);
        break;
    case Type::LIGHTNING:
        FX::drawString(MenuFXData::LIGHTNING);
        break;
    case Type::PLANT:
        FX::drawString(MenuFXData::PLANT);
        break;
    case Type::ELDER:
        FX::drawString(MenuFXData::EARTH);
        break;
    }
}

static void setTextColorBlack() { FX::setFontMode(dcmReverse); }

static void setTextColorWhite() { FX::setFontMode(dcmWhite); }

static void drawInfoRec(uint8_t x, uint8_t y) {
    Arduboy2::fillRect(x - 3, y - 3, 60, 30, WHITE);
    Arduboy2::drawRect(x - 2, y - 2, 58, 28, BLACK);
}

static void printMoveInfo(uint8_t index, uint8_t x, uint8_t y) {
    if (index == 32) {
        return;
    }
    setTextColorBlack();
    drawInfoRec(x, y);
    MoveBitSet m = getMovePack(index);
    FX::setCursor(x, y);

    printType(Type(m.type));
    FX::setCursor(x, y + 8);
    if (m.physical) {
        FX::drawString(MenuFXData::physical);
    } else {
        FX::drawString(MenuFXData::special);
    }
    FX::setCursor(x, y + 16);
    FX::drawString(MenuFXData::power);
    FX::drawNumber(m.power);
}

static void printBattleMenu() {
    FX::setCursor(6, 46);
    FX::drawString(MenuFXData::move);
    FX::setCursor(64, 46);
    FX::drawString(MenuFXData::gather);
    FX::setCursor(6, 54);
    FX::drawString(MenuFXData::change);
    FX::setCursor(64, 54);
    FX::drawString(MenuFXData::escape);
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
    FX::drawString(MenuFXData::pointerText);
}

static void printMoveMenu(int8_t index, uint8_t *moveList) {

    FX::setCursor(6, 46);
    uint24_t rowAddress = FX::readIndexedUInt24(MoveData::moveNames, moveList[0]);
    FX::drawString(rowAddress);
    FX::setCursor(69, 46);
    rowAddress = FX::readIndexedUInt24(MoveData::moveNames, moveList[1]);
    FX::drawString(rowAddress);
    FX::setCursor(6, 55);
    rowAddress = FX::readIndexedUInt24(MoveData::moveNames, moveList[2]);
    FX::drawString(rowAddress);
    FX::setCursor(69, 55);
    rowAddress = FX::readIndexedUInt24(MoveData::moveNames, moveList[3]);
    FX::drawString(rowAddress);
    printMoveInfo(moveList[index], 38, 4);
}

static void printCreatureMenu(uint8_t c1, uint8_t c2, Creature *cpointer) {
    cpointer->printCreature();
    FX::setCursor(6, 45);
    uint24_t addr = FX::readIndexedUInt24(CreatureData::creatureNames, c1);
    FX::drawString(addr);
    FX::setCursor(6, 55);
    addr = FX::readIndexedUInt24(CreatureData::creatureNames, c2);
    FX::drawString(addr);
}