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

inline void expectScriptTextMatches(FxTest &test, const uint8_t *text,
                                    uint8_t length,
                                    const __FlashStringHelper *label) {
    dialogMenu.clear();
    const PopUpDialog scriptText{0, 40, 128, 24, 0, 0, 0, SCRIPT_TEXT, 0};
    dialogMenu.pushMenu(scriptText);
    if (length) memcpy(dialogMenu.scriptText, text, length);
    dialogMenu.head().width = length;

    arduboy.clear();
    dialogMenu.drawPopMenu();
    const uint16_t actual = textSignature();

    arduboy.clear();
    Blit::draw(0, 40, 128, 24, battleMenu, FRAME(0), Blit::OVERWRITE);
    drawRawReference(text, length);
    test.expectEq(actual, textSignature(), label);
}
} // namespace dialogs_test_detail

inline void test_dialogs(FxTest &test) {
    // Exercise the production renderer with SRAM text, using the independent
    // legacy raw glyph declaration for the expected framebuffer.
    static const uint8_t text[] = "Ab0 cD9 eFgHiJkLmNoPqR";
    const uint8_t textLength = sizeof(text) - 1;
    dialogs_test_detail::expectScriptTextMatches(
        test, text, textLength, F("script glyphs match raw font across wrap"));

    static const uint8_t twoRowText[36] = {
        '0', 'z', '!', '{', ' ', 'A', '0', 'z', '!', '{', ' ', 'A',
        '0', 'z', '!', '{', ' ', 'A', '0', 'z', '!', '{', ' ', 'A',
        '0', 'z', '!', '{', ' ', 'A', '0', 'z', '!', '{', ' ', 'A'
    };
    dialogs_test_detail::expectScriptTextMatches(
        test, twoRowText, sizeof(twoRowText),
        F("script glyph endpoints and unsupported bytes match raw font over two rows"));
    dialogs_test_detail::expectScriptTextMatches(
        test, twoRowText, 0, F("empty script text draws no glyphs"));
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
