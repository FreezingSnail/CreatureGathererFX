#pragma once
#include <ArduboyFX.h>
#include "../external/Font4x6.h"
#include <stdint.h>

static void printString(Font4x6 &font, uint24_t addr, uint8_t x, uint8_t y) {
    uint8_t index = 0;
    unsigned char character;
    FX::seekData(addr);
    while ((character = FX::readPendingUInt8()) != '\0') {
        font.printChar(character, x, y);
        x += 5;
    }
    (void)FX::readEnd();
}
