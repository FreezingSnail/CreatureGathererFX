#pragma once

#include <stdint.h>

#include "test.hpp"
#include "../src/common.hpp"
#include "../src/save/Compaction.hpp"
#include "../src/save/FlashBackend.hpp"
#include "../src/save/SaveController.hpp"

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

    test.assert(FRAME(0), static_cast<uint16_t>(1),
                "interim frame selects plane one");
    test.assert(FRAME(2), static_cast<uint16_t>(7),
                "interim frame preserves three-plane stride");
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
    suite.addTest(test);
}

inline void NativeRendererSuite(TestRunner &runner)
{
    TestSuite suite("Native Arduboy2Base renderer integration");
    Arduboy2NativeRendererTest(suite);
    NativeFrameSavePathTest(suite);
    runner.addTestSuite(suite);
}
