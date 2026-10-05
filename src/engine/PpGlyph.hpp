#pragma once

#include <Arduboy2.h>
#include <stdint.h>

namespace PpGlyph {
// Fixed PP glyph coordinates are inside the 128x64 framebuffer. Pixels are
// only set because the move-menu background is cleared before drawing.
inline void draw(uint8_t x, uint8_t y, uint16_t bits) {
    const uint8_t page = y >> 3;
    for (uint8_t row = 0; row < 5; ++row) {
        const uint8_t py = static_cast<uint8_t>(y + row);
        const uint8_t mask = static_cast<uint8_t>(1u << (py & 7));
        const uint8_t offset = static_cast<uint8_t>((py >> 3) - page) * 128;
        for (uint8_t col = 0; col < 3; ++col) {
            if (bits & 1u)
                Arduboy2Base::sBuffer[offset + x + col] |= mask;
            bits >>= 1;
        }
    }
}

inline void digit(uint8_t x, uint8_t y, uint8_t n) {
    const uint16_t bits = n == 0 ? 0b111101101101111 :
                          n == 1 ? 0b111010010011010 :
                          n == 2 ? 0b111001111100111 :
                                   0b111100111100111;
    draw(x, y, bits);
}
} // namespace PpGlyph
