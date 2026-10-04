#pragma once

#include <stdint.h>

#include <Arduboy2.h>
#include <ArduboyFX.h>
extern Arduboy2Base arduboy;

#include "lib/Blit.hpp"

#include "Animator.hpp"
extern Animator animator;

#include "engine/menu/MenuV2.hpp"
extern MenuV2 menu;

#include "engine/ModeState.hpp"

#define DGF __attribute__((optimize("-O0")))

// Final 1bpp assets use one plane per frame.
#define FRAME(x) (x)
#include "fxdata.h"
static void drawStatNumbers(uint8_t x, uint8_t y, uint8_t number) {
    uint8_t upper = number / 100;
    uint8_t lower = number % 100;
    Blit::draw(x, y, 3, 8, singlenumberswhite, FRAME(upper), Blit::PLUSMASK);
    Blit::draw(x + 4, y, 7, 8, numberswhite, FRAME(lower), Blit::PLUSMASK);
}

static void drawNumbersBlack(uint8_t x, uint8_t y, uint8_t number) {
    uint8_t upper = number / 100;
    uint8_t lower = number % 100;
    Blit::draw(x, y, 3, 8, singlenumbersblack, FRAME(upper), Blit::PLUSMASK);
    Blit::draw(x + 4, y, 7, 8, numbersblack, FRAME(lower), Blit::PLUSMASK);
}
