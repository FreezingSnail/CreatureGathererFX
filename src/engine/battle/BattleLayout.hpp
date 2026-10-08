#pragma once
#include <stdint.h>

namespace battle_layout {
constexpr uint8_t spriteSize = 48, playerX = 80, menuY = 48, menuHeight = 16;
constexpr uint8_t centerX = 48, centerWidth = 32;
constexpr uint16_t spriteStride48 = 48 * 6 * 2;
constexpr uint8_t species48 = 64;
static_assert(spriteStride48 == 576, "48px masked frame stride");
static_assert(playerX + spriteSize == 128, "player sprite fits screen");
static_assert(menuY + menuHeight == 64, "menu reaches bottom edge");
}
