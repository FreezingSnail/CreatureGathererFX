#pragma once
#include "DialogMenu.hpp"
#include "../../fxdata.h"
#include "../draw.h"
#include "../world/Event.hpp"
#include <ArduboyFX.h>
#include "../../common.hpp"

#define WHITETEXT 1

namespace {
// Static widths include spaces retained by the legacy strings parser in
// data/text/strings.txt; generated bitmaps have no dimension prefix.

uint8_t loadScriptText(uint16_t index, uint8_t *text, uint8_t capacity,
                       uint8_t &lookups) {
    const uint16_t count = ReadFXu16(raw_map_text);
    ++lookups;
    if (index >= count) return 0;

    const uint24_t offsetAddress = raw_map_text + 2 +
                                  static_cast<uint24_t>(2) * index;
    const uint16_t textOffset = ReadFXu16(offsetAddress);
    ++lookups;
    const uint24_t textStart = raw_map_text + 2 +
                               static_cast<uint24_t>(2) * count;
    const uint24_t lengthAddress = textStart + textOffset;
    const uint16_t textLength = ReadFXu16(lengthAddress);
    ++lookups;

    // The dialog has room for two lines of eighteen 5-pixel glyphs plus one
    // pixel of spacing. Keep text in a small local buffer; sBuffer remains the
    // display framebuffer and is touched only by the normal sprite renderer.
    const uint8_t renderedLength = textLength < capacity
        ? static_cast<uint8_t>(textLength)
        : capacity;
    if (renderedLength == 0) return 0;
    FxRead::bytes(lengthAddress + 2, text, renderedLength);
    ++lookups;
    return renderedLength;
}

void drawScriptText(const uint8_t *text, uint8_t renderedLength, int16_t x, int16_t y) {

    for (uint8_t i = 0; i < renderedLength; ++i) {
        const uint8_t line = i / 18;
        const uint8_t column = i % 18;
        const uint8_t character = text[i];
        if (character >= '0' && character <= 'z') {
            const uint8_t glyph = character - '0';
            Blit::draw(x + column * 6, y + line * 8,
                                      5, 6, fontTrimmed + 4, FRAME(glyph), Blit::OVERWRITE);
        }
    }
}
} // namespace

void DialogMenu::prepareHead() {
    if (!peek()) return;
    if (head().type == SCRIPT_TEXT) {
        uint8_t lookups = 0;
        head().width = loadScriptText(static_cast<uint16_t>(head().textAddress),
                                      scriptText, sizeof(scriptText), lookups);
        FxReadCounter::transitionExact(lookups);
    } else if (head().type == TEXT) {
        const FxRead::SpriteHeader header = FxRead::spriteHeader(head().textAddress);
        head().width = header.width;
        head().height = header.height;
        FxReadCounter::transitionExact(1);
    }
}

void DialogMenu::drawPopMenu() {
    if (!peek()) return;
    PopUpDialog curMenu = head();
    Blit::draw(0, 40, 128, 24, battleMenu, FRAME(0), Blit::OVERWRITE);

    switch (curMenu.type) {
    case TEXT: {
        // Event::load supplies a direct address in the separate raw ASCII event format.
        Blit::draw(curMenu.x + 8, curMenu.y + 2,
                                  curMenu.width, curMenu.height, curMenu.textAddress, FRAME(WHITETEXT), Blit::OVERWRITE);
        break;
    }
    case SCRIPT_TEXT: {
        drawScriptText(scriptText, curMenu.width, curMenu.x + 8, curMenu.y + 2);
        break;
    }
    default:
        break;
    }
}
