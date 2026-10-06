#include "Blit.hpp"
#include <Arduboy2.h>
#ifdef TEST
#include "../../tst/src/FXDataFake.hpp"
#else
#include <ArduboyFX.h>
#endif

namespace Blit {
void draw(int16_t x, int16_t y, uint8_t w, uint8_t h, uint24_t image,
          uint16_t frame, uint8_t mode) {
    if (!w || !h || x >= 128 || y >= 64 || x + w <= 0 || y + h <= 0) return;
    const uint8_t pages = (static_cast<uint16_t>(h) + 7) >> 3;
    const uint8_t bpp = mode == PLUSMASK ? 2 : 1;
    const uint16_t stride = static_cast<uint16_t>(w) * pages * bpp;
    image += static_cast<uint24_t>(frame) * stride;
    const uint8_t first = x < 0 ? -x : 0;
    const uint8_t end = x + w > 128 ? 128 - x : w;
    const uint8_t shift = y & 7;
    // Clipping above bounds y to (-h, 64), so its page fits in int8_t.
    const int8_t page = y >> 3;
    for (uint8_t p = 0; p < pages; ++p) {
        const int8_t d = page + p;
        if (d < -1 || d >= 8 || (d == -1 && !shift)) continue;
        const uint16_t offset = (static_cast<uint16_t>(p) * w + first) * bpp;
        uint24_t address = image + offset;
        const uint8_t heightMask = p == pages - 1 && (h & 7)
            ? (1u << (h & 7)) - 1 : 0xFF;
#ifndef TEST
        FX::seekData(address);
#endif
        for (uint8_t c = first; c < end; ++c) {
            uint8_t pixels, mask = heightMask;
#ifdef TEST
            FX::readDataBytes(address++, &pixels, 1);
            if (mode == PLUSMASK) FX::readDataBytes(address++, &mask, 1);
#else
            pixels = FX::readPendingUInt8();
            if (mode == PLUSMASK) mask = FX::readPendingUInt8();
#endif
            mask &= heightMask;
            if (mode == NEGATIVE) pixels = static_cast<uint8_t>(~pixels);
            const uint16_t bits = static_cast<uint16_t>(pixels & mask) << shift;
            const uint16_t masks = static_cast<uint16_t>(mask) << shift;
            const uint8_t column = x + c;
            if (d >= 0) {
                uint8_t &dest = Arduboy2Base::sBuffer[d * 128 + column];
                dest = (dest & ~static_cast<uint8_t>(masks)) | static_cast<uint8_t>(bits);
            }
            if (shift && d < 7) {
                uint8_t &dest = Arduboy2Base::sBuffer[(d + 1) * 128 + column];
                dest = (dest & ~(masks >> 8)) | (bits >> 8);
            }
        }
#ifndef TEST
        FX::readEnd();
#endif
    }
}

void fillRect(int16_t x, int16_t y, uint8_t w, uint8_t h, uint8_t color) {
    if (!w || !h || x >= 128 || y >= 64 || x + w <= 0 || y + h <= 0) return;
    int16_t right = x + w, bottom = y + h;
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (right > 128) right = 128;
    if (bottom > 64) bottom = 64;
    for (uint8_t page = y >> 3; page <= ((bottom - 1) >> 3); ++page) {
        uint8_t mask = 0xFF;
        if (page == (y >> 3)) mask &= 0xFF << (y & 7);
        if (page == ((bottom - 1) >> 3)) mask &= 0xFF >> (7 - ((bottom - 1) & 7));
        for (int16_t c = x; c < right; ++c) {
            uint8_t &dest = Arduboy2Base::sBuffer[page * 128 + c];
            if (color) dest |= mask;
            else dest &= ~mask;
        }
    }
}
}
