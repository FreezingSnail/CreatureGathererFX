#pragma once

#include <stdint.h>

#define ABG_TIMER1
#define ABG_SYNC_PARK_ROW

#include "external/ArduboyG.h"
extern ArduboyGBase_Config<ABG_Mode::L4_Triplane> arduboy;

#define SPRITESU_OVERWRITE
#define SPRITESU_PLUSMASK
#define SPRITESU_RECT
#define SPRITESU_FX
#include "external/SpritesU.hpp"

#include "Animator.hpp"
extern Animator animator;

#define FRAME(x) x * 3 + arduboy.currentPlane()

#define DGF __attribute__((optimize("-O0")))

#include "fxdata.h"
static void drawStatNumbers(uint8_t x, uint8_t y, uint8_t number) {
    uint8_t upper = number / 100;
    uint8_t lower = number % 100;
    SpritesU::drawPlusMaskFX(x, y, singlenumberswhite, FRAME(upper));
    SpritesU::drawPlusMaskFX(x + 4, y, numberswhite, FRAME(lower));
}

static void drawNumbersBlack(uint8_t x, uint8_t y, uint8_t number) {
    uint8_t upper = number / 100;
    uint8_t lower = number % 100;
    SpritesU::drawPlusMaskFX(x, y, singlenumbersblack, FRAME(upper));
    SpritesU::drawPlusMaskFX(x + 4, y, numbersblack, FRAME(lower));
}