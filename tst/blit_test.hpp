#pragma once
#include <string.h>
#include "test.hpp"
#include "src/FXDataFake.hpp"
#include "../src/lib/Blit.hpp"
#include <Arduboy2.h>

inline void BlitSuite(TestRunner &runner) {
    TestSuite suite("Headerless blitter pixel reference");
    Test test("clipping, masks, fractional pages, frame strides, zero dimensions");
    const int16_t xs[] = {-32768,-16,-5,-1,0,3,127,128,32767};
    const int16_t ys[] = {-32768,-16,-7,-1,0,3,57,60,61,64,32767};
    const uint8_t heights[] = {1,6,8,13,16};
    uint8_t expected[1024];
    for (uint8_t i = 0; i < sizeof(fxDataFake::dataBytes); ++i)
        fxDataFake::dataBytes[i] = static_cast<uint8_t>(i * 37 + 0xD3);
    for (uint8_t mode = 0; mode < 2; ++mode) {
        for (uint8_t h : heights) {
            const uint8_t w = 5, pages = (h + 7) / 8, bpp = mode ? 2 : 1;
            const uint24_t image = 0x123456;
            const uint16_t frame = 4096; // frame * stride exceeds 16 bits.
            fxDataFake::dataBase = image + static_cast<uint24_t>(frame) * w * pages * bpp;
            for (int16_t x : xs) for (int16_t y : ys) {
                memset(expected, 0xA5, sizeof(expected));
                memset(Arduboy2Base::sBuffer, 0xA5, 1024);
                for (uint8_t sy = 0; sy < h; ++sy) for (uint8_t sx = 0; sx < w; ++sx) {
                    const int32_t dx = static_cast<int32_t>(x) + sx;
                    const int32_t dy = static_cast<int32_t>(y) + sy;
                    if (dx < 0 || dx >= 128 || dy < 0 || dy >= 64) continue;
                    const uint8_t offset = ((sy / 8) * w + sx) * bpp;
                    if (mode && !(fxDataFake::dataBytes[offset + 1] & (1 << (sy & 7)))) continue;
                    uint8_t &dest = expected[(dy / 8) * 128 + dx];
                    const uint8_t bit = 1 << (dy & 7);
                    if (fxDataFake::dataBytes[offset] & (1 << (sy & 7))) dest |= bit;
                    else dest &= ~bit;
                }
                Blit::draw(x,y,w,h,image,frame,mode);
                test.assert(memcmp(expected, Arduboy2Base::sBuffer, 1024), 0, "draw matches pixel reference");
            }
        }
    }
    for (uint8_t color = 0; color < 2; ++color) {
        for (int16_t x : xs) for (int16_t y : ys) {
            memset(expected, 0xA5, sizeof(expected));
            memset(Arduboy2Base::sBuffer, 0xA5, 1024);
            for (int16_t sy = 0; sy < 13; ++sy) for (int16_t sx = 0; sx < 20; ++sx) {
                const int32_t dx = static_cast<int32_t>(x) + sx, dy = static_cast<int32_t>(y) + sy;
                if (dx < 0 || dx >= 128 || dy < 0 || dy >= 64) continue;
                uint8_t &dest = expected[(dy / 8) * 128 + dx];
                if (color) dest |= 1 << (dy & 7);
                else dest &= ~(1 << (dy & 7));
            }
            Blit::fillRect(x,y,20,13,color);
            test.assert(memcmp(expected, Arduboy2Base::sBuffer, 1024), 0, "rectangle matches pixel reference");
        }
    }
    memset(Arduboy2Base::sBuffer, 0xA5, 1024);
    memcpy(expected, Arduboy2Base::sBuffer, 1024);
    fxDataFake::readCount = 0;
    Blit::draw(10,10,0,8,0,0,0);
    Blit::draw(10,10,5,0,0,0,0);
    Blit::fillRect(10,10,0,8,WHITE);
    Blit::fillRect(10,10,5,0,BLACK);
    test.assert(memcmp(expected, Arduboy2Base::sBuffer, 1024), 0, "zero dimensions preserve buffer");
    test.assert(fxDataFake::readCount, 0u, "zero dimensions do not read FX");
    fxDataFake::dataBase = 0;
    fxDataFake::readCount = 0;
    memset(fxDataFake::dataBytes, 0, sizeof(fxDataFake::dataBytes));
    suite.addTest(test);
    runner.addTestSuite(suite);
}
