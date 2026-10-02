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

inline bool pixelIsInk(uint8_t x, uint8_t y) {
    const uint8_t *buffer = arduboy.getBuffer();
    const uint16_t index = static_cast<uint16_t>(x + (y / 8) * WIDTH);
    return (buffer[index] & static_cast<uint8_t>(1 << (y % 8))) == 0;
}

inline void drawDialog(DialogType type, uint8_t creatureId = 0, uint16_t moveId = 0) {
    arduboy.clear();
    arduboy.fillScreen(WHITE);
    dialogMenu.clear();
    dialogMenu.pushMenu(newDialogBox(type, creatureId, moveId));
    dialogMenu.drawPopMenu();
}
} // namespace dialog_fx_test_detail

inline void test_dialogs(FxTest &test) {
    arduboy.clear();
    arduboy.fillScreen(WHITE);
    dialogMenu.clear();
    const PopUpDialog damage{20, 10, 120, 30, 0, 0, 42, DAMAGE, 0};
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

    arduboy.clear();
    arduboy.fillScreen(WHITE);
    SpritesU::drawOverwriteFX(0, 40, 128, 24, battleMenu - 2, FRAME(0));
    test.expectEq(dialog_fx_test_detail::regionHasPixel(8, 43, 58, 7, 0), false,
                  F("top text row starts blank"));
    test.expectEq(dialog_fx_test_detail::regionHasPixel(8, 53, 58, 7, 0), false,
                  F("message row starts blank"));
    test.expectEq(dialog_fx_test_detail::regionHasPixel(88, 53, 35, 7, 0), false,
                  F("move row starts blank"));
    test.expectEq(dialog_fx_test_detail::pixelIsInk(8, 46), false,
                  F("name sample starts blank"));
    test.expectEq(dialog_fx_test_detail::pixelIsInk(88, 54), false,
                  F("move sample starts blank"));
    test.expectEq(dialog_fx_test_detail::pixelIsInk(8, 57), false,
                  F("effect sample starts blank"));

    // The generated symbols check the exact table entries. Pixel checks use
    // the original draw coordinates, away from the dialog border.
    dialog_fx_test_detail::drawDialog(NAME, 0, 1);
    test.expectEq(dialogMenu.head().textAddress, creature0, F("name resolved from creature table"));
    test.expectEq(dialogMenu.head().detailAddress, amove1, F("move resolved from move table"));
    test.expectEq(dialogMenu.head().width, 70, F("name bitmap width"));
    test.expectEq(dialogMenu.head().height, 40, F("move bitmap width"));
    test.expectEq(dialog_fx_test_detail::pixelIsInk(8, 46), true,
                  F("name fixed glyph pixel"));
    test.expectEq(dialog_fx_test_detail::pixelIsInk(8, 45), false,
                  F("name fixed paper pixel"));
    test.expectEq(dialog_fx_test_detail::pixelIsInk(88, 54), true,
                  F("move fixed glyph pixel"));
    test.expectEq(dialog_fx_test_detail::pixelIsInk(88, 53), false,
                  F("move fixed paper pixel"));
    test.expectEq(dialog_fx_test_detail::regionHasPixel(8, 45, 58, 7, 0), true,
                  F("name creature row has ink"));
    test.expectEq(dialog_fx_test_detail::regionHasPixel(88, 53, 35, 7, 0), true,
                  F("name move row has ink"));

    dialog_fx_test_detail::drawDialog(NAME, 0, 32);
    test.expectEq(dialogMenu.head().detailAddress, amove32, F("empty move resolves to empty table entry"));
    test.expectEq(dialogMenu.head().height, 0, F("empty move has no bitmap width"));

    dialog_fx_test_detail::drawDialog(ENEMY_NAME, 0, 1);
    test.expectEq(dialogMenu.head().textAddress, creature0, F("enemy name resolved"));
    test.expectEq(dialogMenu.head().detailAddress, amove1, F("enemy move resolved"));
    test.expectEq(dialogMenu.head().width, 70, F("enemy name bitmap width"));
    test.expectEq(dialogMenu.head().height, 40, F("enemy move bitmap width"));
    test.expectEq(dialog_fx_test_detail::regionHasPixel(8, 43, 58, 7, 0), true,
                  F("enemy creature row has ink"));
    test.expectEq(dialog_fx_test_detail::regionHasPixel(88, 53, 35, 7, 0), true,
                  F("enemy move row has ink"));

    dialog_fx_test_detail::drawDialog(FAINT);
    test.expectEq(dialogMenu.head().textAddress, creature0, F("faint name resolved"));
    test.expectEq(dialogMenu.head().width, 70, F("faint name bitmap width"));
    test.expectEq(dialog_fx_test_detail::regionHasPixel(8, 43, 58, 7, 0), true,
                  F("faint creature row has ink"));
    test.expectEq(dialog_fx_test_detail::regionHasPixel(8, 53, 58, 7, 0), true,
                  F("faint message row has ink"));

    dialog_fx_test_detail::drawDialog(SWITCH);
    test.expectEq(dialogMenu.head().textAddress, creature0, F("switch name resolved"));
    test.expectEq(dialogMenu.head().width, 70, F("switch name bitmap width"));
    test.expectEq(dialog_fx_test_detail::regionHasPixel(8, 43, 58, 7, 0), true,
                  F("switch creature row has ink"));
    test.expectEq(dialog_fx_test_detail::regionHasPixel(8, 53, 58, 7, 0), true,
                  F("switch message row has ink"));

    dialog_fx_test_detail::drawDialog(WIN);
    test.expectEq(dialog_fx_test_detail::regionHasPixel(8, 53, 58, 7, 0), true,
                  F("win message row has ink"));

    dialog_fx_test_detail::drawDialog(LOSS);
    test.expectEq(dialogMenu.head().textAddress, creature0, F("loss name resolved"));
    test.expectEq(dialogMenu.head().width, 70, F("loss name bitmap width"));
    test.expectEq(dialog_fx_test_detail::regionHasPixel(8, 43, 58, 7, 0), true,
                  F("loss creature row has ink"));
    test.expectEq(dialog_fx_test_detail::regionHasPixel(8, 53, 58, 7, 0), true,
                  F("loss message row has ink"));

    dialog_fx_test_detail::drawDialog(PLAYER_EFFECT, 3);
    test.expectEq(dialogMenu.head().textAddress, creature3, F("player effect name resolved"));
    test.expectEq(dialogMenu.head().detailAddress, applied, F("player effect text resolved"));
    test.expectEq(dialogMenu.head().width, 60, F("player effect name bitmap width"));
    test.expectEq(dialogMenu.head().height, 75, F("player effect text bitmap width"));
    test.expectEq(dialog_fx_test_detail::pixelIsInk(8, 44), true,
                  F("player effect name fixed glyph pixel"));
    test.expectEq(dialog_fx_test_detail::pixelIsInk(8, 57), true,
                  F("player effect text fixed glyph pixel"));
    test.expectEq(dialog_fx_test_detail::pixelIsInk(8, 53), false,
                  F("player effect text fixed paper pixel"));
    test.expectEq(dialog_fx_test_detail::regionHasPixel(8, 43, 58, 7, 0), true,
                  F("player effect creature row has ink"));
    test.expectEq(dialog_fx_test_detail::regionHasPixel(8, 53, 58, 7, 0), true,
                  F("player effect message row has ink"));

    dialog_fx_test_detail::drawDialog(ENEMY_EFFECT, 3);
    test.expectEq(dialogMenu.head().textAddress, creature3, F("enemy effect name resolved"));
    test.expectEq(dialogMenu.head().detailAddress, applied, F("enemy effect text resolved"));
    test.expectEq(dialogMenu.head().width, 60, F("enemy effect name bitmap width"));
    test.expectEq(dialogMenu.head().height, 75, F("enemy effect text bitmap width"));
    test.expectEq(dialog_fx_test_detail::pixelIsInk(8, 44), true,
                  F("enemy effect name fixed glyph pixel"));
    test.expectEq(dialog_fx_test_detail::pixelIsInk(8, 57), true,
                  F("enemy effect text fixed glyph pixel"));
    test.expectEq(dialog_fx_test_detail::pixelIsInk(8, 53), false,
                  F("enemy effect text fixed paper pixel"));
    test.expectEq(dialog_fx_test_detail::regionHasPixel(8, 43, 58, 7, 0), true,
                  F("enemy effect creature row has ink"));
    test.expectEq(dialog_fx_test_detail::regionHasPixel(8, 53, 58, 7, 0), true,
                  F("enemy effect message row has ink"));
}
