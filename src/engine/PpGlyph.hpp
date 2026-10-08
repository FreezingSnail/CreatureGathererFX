#pragma once

#include <Arduboy2.h>
#include <avr/pgmspace.h>
#include <stdint.h>

namespace PpGlyph {
// Compact menu/HUD glyph coordinates are inside the 128x64 framebuffer.
// Pixels are only set; callers clear the background before drawing.
inline void draw(uint8_t x, uint8_t y, uint16_t bits) {
    for (uint8_t row = 0; row < 5; ++row) {
        const uint8_t py = static_cast<uint8_t>(y + row);
        const uint8_t mask = static_cast<uint8_t>(1u << (py & 7));
        const uint16_t offset = static_cast<uint16_t>(py >> 3) * 128;
        for (uint8_t col = 0; col < 3; ++col) {
            if (bits & 1u)
                Arduboy2Base::sBuffer[offset + x + col] |= mask;
            bits >>= 1;
        }
    }
}

inline void digit(uint8_t x, uint8_t y, uint8_t n) {
    static const uint16_t glyphs[] PROGMEM = {
        0x7b6f, 0x749a, 0x73e7, 0x79e7, 0x49ed, 0x79cf, 0x7bcf, 0x24a7, 0x7bef, 0x79ef
    };
    if (n < 10) draw(x, y, pgm_read_word(glyphs + n));
}

inline uint8_t number(uint8_t x, uint8_t y, uint8_t n) {
    uint8_t divisor = n >= 100 ? 100 : n >= 10 ? 10 : 1;
    do {
        digit(x, y, n / divisor);
        x += 4;
        n %= divisor;
        divisor /= 10;
    } while (divisor);
    return x;
}
} // namespace PpGlyph
