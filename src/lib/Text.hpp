#pragma once
#include <ArduboyFX.h>
#include "../external/Font4x6.h"
#include <stdint.h>

static void printString(Font4x6 &font, uint24_t addr, uint8_t x, uint8_t y) {
    uint8_t index = 0;
    unsigned char character;
    do {
        character = FX::readIndexedUInt8(addr, index);
        font.printChar(character, x, y);
        index++;
        x += 5;
    } while (character != '\0');
}
