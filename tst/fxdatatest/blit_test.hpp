#pragma once
#include <string.h>
#include "fxtest.hpp"
#include "src/fxdata.h"
#include "src/lib/Blit.hpp"
#include "src/lib/FxRead.hpp"

namespace blit_test {
struct DrawCase { int16_t x, y; uint8_t w, h; uint24_t image; uint16_t frame; uint8_t mode; };
constexpr uint16_t lastTile = (maskedFont - tiles) / 32 - 1;
#define TILE_CASES(f) {0,0,16,16,tiles,f,0}, {-15,0,16,16,tiles,f,0}, \
 {127,0,16,16,tiles,f,0}, {0,-7,16,16,tiles,f,0}, {0,3,16,16,tiles,f,0}, \
 {0,60,16,16,tiles,f,0}, {-15,-15,16,16,tiles,f,0}
const DrawCase draws[] PROGMEM = {
    TILE_CASES(0), TILE_CASES(lastTile),
    {56,24,16,16,characterSheet,0,0},
    {0,0,32,32,NewecreatureSprites,0,1}, {96,0,32,32,NewecreatureSprites,0,1},
    {-31,5,32,32,NewecreatureSprites,0,1}, {100,40,32,32,NewecreatureSprites,0,1},
    {0,40,128,24,battleMenu,0,0}, {10,57,75,8,doubled,0,0},
    {-3,-3,60,32,moveInfo,0,0},
    {0,0,5,6,fontTrimmed,17,0}, {3,61,5,6,fontTrimmed,17,0}
};
#undef TILE_CASES
struct RectCase { int16_t x,y; uint8_t w,h,color; };
const RectCase rects[] PROGMEM = {
    {0,0,128,64,WHITE}, {-5,-5,10,10,BLACK}, {120,60,20,20,WHITE},
    {10,10,0,5,WHITE}, {10,3,7,13,BLACK}
};
inline uint16_t crc() {
    uint16_t value = 0xFFFF;
    for (uint16_t i = 0; i < 1024; ++i) {
        value ^= Arduboy2Base::sBuffer[i];
        for (uint8_t bit = 0; bit < 8; ++bit)
            value = (value >> 1) ^ ((value & 1) ? 0xA001 : 0);
    }
    return value;
}
// CRC-16/Modbus, initial 0xFFFF, captured from full-buffer SpritesU parity.
// Fractional-height glyph cases use the independent pixel reference because
// the retired renderer discarded h < 8 (including the frame stride).
const uint16_t drawCrcs[] PROGMEM = {
    54462,54462,54462,54462,54462,54462,54462,
    54462,54462,54462,54462,54462,54462,54462,
    62304,57228,12025,1293,11519,23821,29400,4863,48110,34405
};
const uint16_t rectCrcs[] PROGMEM = {45310,33433,2296,16460,26057};
inline void run(FxTest &test) {
    for (uint8_t i = 0; i < sizeof(draws) / sizeof(draws[0]); ++i) {
        DrawCase c;
        memcpy_P(&c, draws + i, sizeof(c));
        const uint8_t background = c.mode ? 0xA5 : 0;
        memset(Arduboy2Base::sBuffer, background, 1024);
        Blit::draw(c.x,c.y,c.w,c.h,c.image,c.frame,c.mode);
        test.expectEqIdx(crc(), pgm_read_word(drawCrcs + i), F("draw CRC"), i);
    }
    for (uint8_t i = 0; i < sizeof(rects) / sizeof(rects[0]); ++i) {
        RectCase c;
        memcpy_P(&c, rects + i, sizeof(c));
        memset(Arduboy2Base::sBuffer, 0xA5, 1024);
        Blit::fillRect(c.x,c.y,c.w,c.h,c.color);
        test.expectEqIdx(crc(), pgm_read_word(rectCrcs + i), F("rectangle CRC"), i);
    }
}
}
