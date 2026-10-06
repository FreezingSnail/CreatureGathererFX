#include "Text.hpp"
#include "../common.hpp"
#include "../fxdata.h"
#ifdef __AVR__
__attribute__((noinline))
#endif
void drawText(int16_t x, int16_t y, uint24_t address, uint8_t width, uint8_t frame) {
    if (width && address) Blit::draw(x, y, width, 8, address, frame, Blit::OVERWRITE);
}

void drawGlyph(int16_t x, int16_t y, uint8_t character, uint8_t mode) {
    if (character < '0' || character > 'z') return;
    Blit::draw(x, y, 5, 6, fontTrimmed + 4, FRAME(character - '0'), mode);
}
