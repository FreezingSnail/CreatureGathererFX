#pragma once

#include <string.h>

#include "fxtest.hpp"
#include "src/fxdata.h"
#include "src/engine/draw.h"
#include "src/engine/menu/DialogMenu.hpp"

namespace dialogs_test_detail {
inline uint16_t textSignature() {
    uint16_t hash = 1;
    for (uint8_t page = 5; page < 8; ++page)
        for (uint8_t x = 0; x < 128; ++x)
            hash = static_cast<uint16_t>(hash * 33u + arduboy.getBuffer()[page * 128u + x]);
    return hash;
}

inline void drawRawReference(const uint8_t *text, uint8_t length) {
    for (uint8_t index = 0; index < length; ++index) {
        const uint8_t character = text[index];
        if (character == ' ') continue;
        if (character >= '0' && character <= 'z')
            Blit::draw(8 + (index % 18) * 6, 42 + (index / 18) * 8, 5, 6,
                       ArduFontTrimmed, FRAME(character - '0'), Blit::OVERWRITE);
    }
}
} // namespace dialogs_test_detail

inline void test_dialogs(FxTest &test) {
    dialogMenu.clear();

    // Exercise the production renderer with SRAM text, using the independent
    // legacy raw glyph declaration for the expected framebuffer.
    static const uint8_t text[] = "Ab0 cD9 eFgHiJkLmNoPqR";
    const uint8_t textLength = sizeof(text) - 1;
    const PopUpDialog scriptText{0, 40, 128, 24, 0, 0, 0, SCRIPT_TEXT, 0};
    dialogMenu.pushMenu(scriptText);
    memcpy(dialogMenu.scriptText, text, textLength);
    dialogMenu.head().width = textLength;
    arduboy.clear();
    dialogMenu.drawPopMenu();
    const uint16_t actual = dialogs_test_detail::textSignature();

    arduboy.clear();
    Blit::draw(0, 40, 128, 24, battleMenu, FRAME(0), Blit::OVERWRITE);
    dialogs_test_detail::drawRawReference(text, textLength);
    test.expectEq(actual, dialogs_test_detail::textSignature(),
                  F("script glyphs match independent raw font across wrap"));
    dialogMenu.clear();

    const PopUpDialog invalidScriptText{0, 43, 128, 24, 0xFFFF, 0, 0,
                                        SCRIPT_TEXT, 0};
    test.expectEq(dialogMenu.push(invalidScriptText), true,
                  F("script text dialog queued"));
    test.expectEq(dialogMenu.head().type, SCRIPT_TEXT,
                  F("script text type retained"));
    test.expectEq(dialogMenu.head().width, 0,
                  F("invalid script text resolves to empty"));

    // The current generated map has no script text entries; scripts_test
    // exercises real Msg dispatch when data provides one.
    dialogMenu.drawPopMenu();
    test.expectEq(dialogMenu.peek(), true,
                  F("drawing leaves queued dialog in place"));
    test.expectEq(dialogMenu.head().textAddress, static_cast<uint24_t>(0xFFFF),
                  F("drawing keeps the prepared head"));

    dialogMenu.clear();
    test.expectEq(dialogMenu.peek(), false, F("clear empties world dialog queue"));
}
