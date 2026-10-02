#pragma once

#include "fxtest.hpp"
#include "src/engine/menu/DialogMenu.hpp"

namespace dialog_fx_test_detail {
inline bool regionHasPixel(uint8_t x, uint8_t y, uint8_t width, uint8_t height,
                           uint8_t expected) {
    const uint8_t *buffer = arduboy.getBuffer();
    for (uint8_t row = y; row < y + height; ++row) {
        for (uint8_t column = x; column < x + width; ++column) {
            const uint16_t index = static_cast<uint16_t>(column + (row / 8) * WIDTH);
            const uint8_t mask = static_cast<uint8_t>(1 << (row % 8));
            if (((buffer[index] & mask) != 0) == (expected != 0)) {
                return true;
            }
        }
    }
    return false;
}
} // namespace dialog_fx_test_detail

inline void test_dialogs(FxTest &test) {
    arduboy.clear();
    arduboy.fillScreen(WHITE);
    dialogMenu.clear();
    const PopUpDialog damage{20, 10, 120, 30, 0, 42, DAMAGE, 0};
    test.expectEq(dialogMenu.push(damage), true, F("damage dialog queued"));
    dialogMenu.drawPopMenu();

    // The label and number render in separate regions; the damage switch's
    // explicit break keeps DAMAGE from also entering ENEMY_DAMAGE.
    test.expectEq(dialog_fx_test_detail::regionHasPixel(32, 12, 48, 7, 0), true,
                  F("damage label has ink"));
    test.expectEq(dialog_fx_test_detail::regionHasPixel(24, 13, 8, 7, 0), true,
                  F("damage number has ink"));
    test.expectEq(dialog_fx_test_detail::regionHasPixel(5, 5, 4, 4, 1), true,
                  F("outside dialog remains paper"));
}
