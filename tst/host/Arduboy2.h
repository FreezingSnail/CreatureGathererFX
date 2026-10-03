#pragma once

#include <stddef.h>
#include <stdint.h>

#define BLACK 0
#define WHITE 1
#define INVERT 2
#define CLEAR_BUFFER true
#define WIDTH 128
#define HEIGHT 64

#define LEFT_BUTTON (1u << 5)
#define RIGHT_BUTTON (1u << 6)
#define UP_BUTTON (1u << 7)
#define DOWN_BUTTON (1u << 4)
#define A_BUTTON (1u << 3)
#define B_BUTTON (1u << 2)

using __uint24 = uint32_t;

class Arduboy2Base {
  public:
    static constexpr size_t BUFFER_BYTES = (WIDTH * HEIGHT) / 8;

    static uint8_t sBuffer[BUFFER_BYTES];
    static uint16_t frameCount;
    static uint8_t currentButtonState;
    static uint8_t previousButtonState;

    static void clear();
    static void fillScreen(uint8_t color = WHITE);
    static void display();
    static void display(bool clear);
    static void drawPixel(int16_t x, int16_t y, uint8_t color = WHITE);
    static uint8_t getPixel(uint8_t x, uint8_t y);
    static uint8_t *getBuffer();

    static void setFrameRate(uint8_t rate);
    static void setFrameDuration(uint8_t duration);
    static bool nextFrame();
    static bool pressed(uint8_t buttons);
    static bool anyPressed(uint8_t buttons);
    static bool notPressed(uint8_t buttons);
    static void pollButtons();
    static bool justPressed(uint8_t button);
    static bool justReleased(uint8_t button);

    // Deterministic controls/counters keep the production frame contract
    // executable without a clock, GPIO, OLED, or Arduino runtime.
    static void resetForTest();
    static void setFrameReadyForTest(bool ready);
    static void setButtonStateForTest(uint8_t buttons);
    static uint16_t displayCountForTest();
    static uint16_t pollCountForTest();
    static uint8_t frameDurationForTest();
};

using Arduboy2 = Arduboy2Base;

extern Arduboy2Base arduboy;
