#pragma once

#include <stdint.h>

#include <Arduboy2.h>
extern Arduboy2Base arduboy;

#define SPRITESU_OVERWRITE
#define SPRITESU_PLUSMASK
#define SPRITESU_RECT
#define SPRITESU_FX
#include "external/SpritesU.hpp"

#include "Animator.hpp"
extern Animator animator;

#include "engine/menu/MenuV2.hpp"
extern MenuV2 menu;

#include "engine/battle/Battle.hpp"
extern BattleEngine engine;

#define DGF __attribute__((optimize("-O0")))

// Interim: assets still carry 3 grayscale planes per frame; plane 1 is the
// 50% (R >= 128) threshold, matching the upcoming 1bpp (shades = 2) encoding.
#define FRAME(x) ((x) * 3 + 1)
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
    // These FX symbols point at sprite payloads; the explicit-size overload
    // expects the dimensions prefix because it advances past it internally.
    SpritesU::drawPlusMaskFX(x, y, 3, 8, singlenumbersblack - 2, FRAME(upper));
    SpritesU::drawPlusMaskFX(x + 4, y, 7, 8, numbersblack - 2, FRAME(lower));
}
