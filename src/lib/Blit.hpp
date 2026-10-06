#pragma once
#include "uint24.h"

// Addresses point directly at headerless, page-major pixel bytes.
namespace Blit {
enum : uint8_t { OVERWRITE = 0, PLUSMASK = 1, NEGATIVE = 2 };
void draw(int16_t x, int16_t y, uint8_t w, uint8_t h, uint24_t image,
          uint16_t frame, uint8_t mode = OVERWRITE);
void fillRect(int16_t x, int16_t y, uint8_t w, uint8_t h, uint8_t color);
}
