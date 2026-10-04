#include "Text.hpp"
#include "../common.hpp"
#ifdef __AVR__
__attribute__((noinline))
#endif
void drawText(int16_t x, int16_t y, uint24_t address, uint8_t width, uint8_t frame) {
    if (width && address) Blit::draw(x, y, width, 8, address, frame, Blit::OVERWRITE);
}
