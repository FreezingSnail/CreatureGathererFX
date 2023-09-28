#pragma once
#include "../lib/Move.hpp"
#include <ArduboyFX.h>

static void
printMoveInfo(Arduboy2 *arduboy, uint8_t index) {
    if (index == 32) {
        return;
    }
    MoveBitSet m = getMovePack(index);
    arduboy->setCursor(38, 4);

    arduboy->print(readFlashStringPointer(&typeNames[m.type]));
    arduboy->setCursor(38, 12);
    if (m.physical) {
        arduboy->print(F("Phys"));
    } else {
        arduboy->print(F("Spec"));
    }
    arduboy->setCursor(38, 19);
    arduboy->print(F("power:"));
    arduboy->print(m.power);
}