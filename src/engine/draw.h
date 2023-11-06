#pragma once
#include "../external/FlashStringHelper.h"
#include "../lib/Move.hpp"
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