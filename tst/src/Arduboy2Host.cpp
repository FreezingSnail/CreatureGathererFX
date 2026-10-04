#include "Arduboy2.h"

#include "avr/pgmspace.h"
#include "../../src/lib/FxRead.hpp"



#include <cstring>

uint8_t Arduboy2Base::sBuffer[Arduboy2Base::BUFFER_BYTES] = {};
uint16_t Arduboy2Base::frameCount = 0;
uint8_t Arduboy2Base::currentButtonState = 0;
uint8_t Arduboy2Base::previousButtonState = 0;

namespace {
uint8_t frameDuration = 16;
uint8_t inputButtonState = 0;
bool frameReady = false;
uint16_t displayCount = 0;
uint16_t pollCount = 0;
}

Arduboy2Base arduboy;

void Arduboy2Base::clear()
{
    fillScreen(BLACK);
}

void Arduboy2Base::fillScreen(uint8_t color)
{
    std::memset(sBuffer, color == BLACK ? 0 : 0xff, sizeof(sBuffer));
}

void Arduboy2Base::display()
{
    display(false);
}

void Arduboy2Base::display(bool clearBuffer)
{
    ++displayCount;
    if (clearBuffer) clear();
}

void Arduboy2Base::drawPixel(int16_t x, int16_t y, uint8_t color)
{
    if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT) return;
    const size_t offset = static_cast<size_t>(y / 8) * WIDTH + x;
    const uint8_t bit = static_cast<uint8_t>(1u << (y & 7));
    if (color == BLACK) sBuffer[offset] &= static_cast<uint8_t>(~bit);
    else sBuffer[offset] |= bit;
}

uint8_t Arduboy2Base::getPixel(uint8_t x, uint8_t y)
{
    const size_t offset = static_cast<size_t>(y / 8) * WIDTH + x;
    return static_cast<uint8_t>((sBuffer[offset] >> (y & 7)) & 1u);
}

uint8_t *Arduboy2Base::getBuffer()
{
    return sBuffer;
}

void Arduboy2Base::setFrameRate(uint8_t rate)
{
    frameDuration = static_cast<uint8_t>(1000 / rate);
}

void Arduboy2Base::setFrameDuration(uint8_t duration)
{
    frameDuration = duration;
}

bool Arduboy2Base::nextFrame()
{
    if (!frameReady) return false;
    frameReady = false;
    ++frameCount;
    return true;
}

bool Arduboy2Base::pressed(uint8_t buttons)
{
    return (currentButtonState & buttons) == buttons;
}

bool Arduboy2Base::anyPressed(uint8_t buttons)
{
    return (currentButtonState & buttons) != 0;
}

bool Arduboy2Base::notPressed(uint8_t buttons)
{
    return (currentButtonState & buttons) == 0;
}

void Arduboy2Base::pollButtons()
{
    previousButtonState = currentButtonState;
    currentButtonState = inputButtonState;
    ++pollCount;
}

bool Arduboy2Base::justPressed(uint8_t button)
{
    return !(previousButtonState & button) && (currentButtonState & button);
}

bool Arduboy2Base::justReleased(uint8_t button)
{
    return (previousButtonState & button) && !(currentButtonState & button);
}

void Arduboy2Base::resetForTest()
{
    std::memset(sBuffer, 0, sizeof(sBuffer));
    frameCount = 0;
    currentButtonState = 0;
    previousButtonState = 0;
    frameDuration = 16;
    inputButtonState = 0;
    frameReady = false;
    displayCount = 0;
    pollCount = 0;
}

void Arduboy2Base::setFrameReadyForTest(bool ready)
{
    frameReady = ready;
}

void Arduboy2Base::setButtonStateForTest(uint8_t buttons)
{
    inputButtonState = buttons;
}

uint16_t Arduboy2Base::displayCountForTest()
{
    return displayCount;
}

uint16_t Arduboy2Base::pollCountForTest()
{
    return pollCount;
}

uint8_t Arduboy2Base::frameDurationForTest()
{
    return frameDuration;
}
