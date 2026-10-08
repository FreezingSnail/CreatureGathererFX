#pragma once

#include <stdint.h>

#include "test.hpp"
#include "../src/common.hpp"
#include "../src/engine/PpGlyph.hpp"
#include "../src/save/Compaction.hpp"
#include "../src/save/FlashBackend.hpp"
#include "../src/save/SaveController.hpp"
#include "../src/lib/MoveIds.hpp"

namespace renderer_test_detail {
inline uint16_t litPixels()
{
    uint16_t count = 0;
    for (uint8_t y = 0; y < HEIGHT; ++y) {
        for (uint8_t x = 0; x < WIDTH; ++x) {
            count += Arduboy2Base::getPixel(x, y);
        }
    }
    return count;
}

inline void assertPpGlyph(Test &test, uint16_t bits, uint8_t x, uint8_t y,
                          const char *message)
{
    Arduboy2Base::clear();
    PpGlyph::draw(x, y, bits);
    for (uint8_t py = 0; py < HEIGHT; ++py) {
        for (uint8_t px = 0; px < WIDTH; ++px) {
            bool expected = false;
            if (px >= x && px < x + 3 && py >= y && py < y + 5) {
                const uint8_t bit = static_cast<uint8_t>((py - y) * 3 + px - x);
                expected = (bits & (1u << bit)) != 0;
            }
            test.assert(Arduboy2Base::getPixel(px, py), expected, message);
        }
    }
}

inline void assertPpDigit(Test &test, uint8_t digit, uint16_t bits)
{
    Arduboy2Base::clear();
    PpGlyph::digit(12, 4, digit);
    for (uint8_t y = 0; y < HEIGHT; ++y) {
        for (uint8_t x = 0; x < WIDTH; ++x) {
            bool expected = false;
            if (x >= 12 && x < 15 && y >= 4 && y < 9) {
                const uint8_t bit = static_cast<uint8_t>((y - 4) * 3 + x - 12);
                expected = (bits & (1u << bit)) != 0;
            }
            test.assert(Arduboy2Base::getPixel(x, y), expected,
                        "compact digit selector matches authored pixels");
        }
    }
}

inline void assertSaveStatusOracle(Test &test, const uint8_t (*text)[5],
                                   const char *message)
{
    for (uint8_t y = 0; y < HEIGHT; ++y) {
        for (uint8_t x = 0; x < WIDTH; ++x) {
            bool expected = true;
            if (y >= 27 && y < 37 && x >= 34 && x < 94) {
                const uint8_t glyph = static_cast<uint8_t>((x - 34) / 10);
                const uint8_t column = static_cast<uint8_t>(((x - 34) % 10) / 2);
                const uint8_t row = static_cast<uint8_t>((y - 27) / 2);
                expected = (text[glyph][row] & (1u << (4 - column))) == 0;
            }
            test.assert(Arduboy2Base::getPixel(x, y), expected, message);
        }
    }
}

inline void PpGlyphPixelEquivalenceTest(TestSuite &suite)
{
    Test test = Test(__func__);
    const uint16_t digits[10] = {
        0b111101101101111, 0b111010010011010,
        0b111001111100111, 0b111100111100111,
        18925, 31183, 31695, 9383, 31727, 31215,
    };
    for (uint8_t digit = 0; digit < 10; ++digit) {
        if (digit < 4)
            assertPpGlyph(test, digits[digit], 12, 4, "PP digit matches original pixels");
        assertPpDigit(test, digit, digits[digit]);
    }
    test.assert(validMoveId(LEGACY_EMPTY_MOVE_ID), false,
                "legacy empty slot takes PP suppression branch");
    test.assert(validMoveId(EMPTY_MOVE_ID), false,
                "absent slot takes PP suppression branch");
    assertPpGlyph(test, 0b001001111101111, 30, 4, "PP label matches original pixels");
    assertPpGlyph(test, 0b001001010100100, 40, 4, "PP slash matches original pixels");
    assertPpGlyph(test, 0b010111010, 50, 5, "unlimited star matches original pixels");

    suite.addTest(test);
}
}

inline void Arduboy2NativeRendererTest(TestSuite &suite)
{
    Test test = Test(__func__);
    Arduboy2Base::resetForTest();

    Arduboy2Base::fillScreen(WHITE);
    test.assert(renderer_test_detail::litPixels(), static_cast<uint16_t>(WIDTH * HEIGHT),
                "native 1bpp fill lights every pixel");

    Arduboy2Base::clear();
    Arduboy2Base::drawPixel(0, 0, WHITE);
    Arduboy2Base::drawPixel(127, 63, WHITE);
    Arduboy2Base::drawPixel(-1, 0, WHITE);
    Arduboy2Base::drawPixel(128, 63, WHITE);
    test.assert(Arduboy2Base::getPixel(0, 0), WHITE,
                "native renderer sets first pixel");
    test.assert(Arduboy2Base::getPixel(127, 63), WHITE,
                "native renderer sets bottom-right pixel");
    test.assert(Arduboy2Base::sBuffer[0], static_cast<uint8_t>(0x01),
                "native buffer uses page-major first byte");
    test.assert(Arduboy2Base::sBuffer[7 * WIDTH + 127], static_cast<uint8_t>(0x80),
                "native buffer uses page-major last byte");
    Arduboy2Base::drawPixel(0, 0, BLACK);
    test.assert(Arduboy2Base::getPixel(0, 0), BLACK,
                "native renderer clears a pixel");

    test.assert(FRAME(0), static_cast<uint16_t>(0),
                "final 1bpp frame starts at zero");
    test.assert(FRAME(2), static_cast<uint16_t>(2),
                "final 1bpp frame has one entry per frame");
    suite.addTest(test);
}

inline void NativeFrameSavePathTest(TestSuite &suite)
{
    Test test = Test(__func__);
    flashFakeReset();
    Arduboy2Base::resetForTest();
    arduboy.setFrameRate(52);
    test.assert(arduboy.frameDurationForTest(), static_cast<uint8_t>(19),
                "native frame rate is 52 Hz");

    gameState.state = GameState_t::WORLD;
    SaveController::begin(GameState_t::WORLD);
    test.assert(gameState.state, GameState_t::SAVING,
                "save path enters SAVING state");
    test.assert(saveInProgress(), true, "save path starts its flash operation");

    Arduboy2Base::setFrameReadyForTest(true);
    const bool frameAccepted = arduboy.nextFrame();
    test.assert(frameAccepted, true, "one ready update accepts one frame");
    if (frameAccepted) {
        arduboy.pollButtons();
        SaveController::drawStatus();
    }
    static const uint8_t saving[6][5] = {
        {0x0f, 0x10, 0x0e, 0x01, 0x1e}, {0x0e, 0x11, 0x1f, 0x11, 0x11},
        {0x11, 0x11, 0x11, 0x0a, 0x04}, {0x1f, 0x04, 0x04, 0x04, 0x1f},
        {0x11, 0x19, 0x15, 0x13, 0x11}, {0x0e, 0x10, 0x17, 0x11, 0x0e},
    };
    renderer_test_detail::assertSaveStatusOracle(test, saving,
                                                "SAVING screen matches original pixels");
    const uint16_t drawnPixels = renderer_test_detail::litPixels();
    test.assert(drawnPixels > 0, true, "save screen paints native framebuffer");
    test.assert(drawnPixels < static_cast<uint16_t>(WIDTH * HEIGHT), true,
                "save screen paints glyph cutouts over its background");
    test.assert(Arduboy2Base::getPixel(36, 27), BLACK,
                "save screen draws the S glyph through native renderer");
    test.assert(Arduboy2Base::getPixel(0, 0), WHITE,
                "save screen fills native framebuffer background");
    test.assert(Arduboy2Base::pollCountForTest(), static_cast<uint16_t>(1),
                "frame polls buttons once");

    arduboy.display(CLEAR_BUFFER);
    test.assert(renderer_test_detail::litPixels(), static_cast<uint16_t>(0),
                "display with CLEAR_BUFFER clears after one frame");
    test.assert(Arduboy2Base::displayCountForTest(), static_cast<uint16_t>(1),
                "frame displays once");
    test.assert(arduboy.nextFrame(), false,
                "second update does not render without a new frame");

    for (uint8_t step = 0; step < 64 && saveInProgress(); ++step) {
        SaveController::advance();
    }
    test.assert(saveInProgress(), false, "save path completes through frame advances");
    test.assert(gameState.state, GameState_t::WORLD,
                "save path restores its return state");
    static const uint8_t failed[6][5] = {
        {0x1f, 0x10, 0x1e, 0x10, 0x10}, {0x0e, 0x11, 0x1f, 0x11, 0x11},
        {0x1f, 0x04, 0x04, 0x04, 0x1f}, {0x10, 0x10, 0x10, 0x10, 0x1f},
        {0x1f, 0x10, 0x1e, 0x10, 0x1f}, {0x1e, 0x11, 0x11, 0x11, 0x1e},
    };
    SaveController::drawStatus();
    renderer_test_detail::assertSaveStatusOracle(test, failed,
                                                "FAILED screen matches original pixels");
    suite.addTest(test);
}

inline void NativeRendererSuite(TestRunner &runner)
{
    TestSuite suite("Native Arduboy2Base renderer integration");
    Arduboy2NativeRendererTest(suite);
    renderer_test_detail::PpGlyphPixelEquivalenceTest(suite);
    NativeFrameSavePathTest(suite);
    runner.addTestSuite(suite);
}
