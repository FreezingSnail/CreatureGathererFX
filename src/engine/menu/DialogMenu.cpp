#pragma once
#include "DialogMenu.hpp"
#include "../../fxdata.h"
#include "../../lib/Type.hpp"
#include "../draw.h"
#include "../world/Event.hpp"
#include <ArduboyFX.h>
#include "../../common.hpp"

#define WHITETEXT 1
#define BLACKTEXT 0

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
                                      5, 6, fontTrimmed, FRAME(glyph), Blit::OVERWRITE);
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
    case DAMAGE: {
        drawText(curMenu.x + 12, curMenu.y + 2, damageText, 70, FRAME(WHITETEXT));
        drawNumbersBlack(curMenu.x + 4, curMenu.y + 3, curMenu.damage);
        break;
    }
    case ENEMY_DAMAGE: {
        // font.setCursor(curMenu.x + 3, curMenu.y + 3);
        // font.println(curMenu.damage);
        drawText(curMenu.x + 12, curMenu.y + 2, damageText, 70, FRAME(WHITETEXT));
        drawNumbersBlack(curMenu.x + 4, curMenu.y + 3, curMenu.damage);
        break;
    }
    case NAME: {
        drawText(curMenu.x + 3, curMenu.y + 2, curMenu.textAddress, curMenu.width, FRAME(WHITETEXT));
        drawText(curMenu.x + 3, curMenu.y + 10, attackText, 90, FRAME(WHITETEXT));
        if (curMenu.damage != 0) {
            drawText(curMenu.x + 83, curMenu.y + 10, curMenu.detailAddress, curMenu.height, FRAME(WHITETEXT));
        }
        break;
    }
    case ENEMY_NAME: {
        drawText(curMenu.x + 3, curMenu.y, curMenu.textAddress, curMenu.width, FRAME(WHITETEXT));
        drawText(curMenu.x + 3, curMenu.y + 10, enemyAttackText, 70, FRAME(WHITETEXT));
        if (curMenu.damage != 0) {
            drawText(curMenu.x + 83, curMenu.y + 10, curMenu.detailAddress, curMenu.height, FRAME(WHITETEXT));
        }
        break;
    }
    case FAINT: {
        drawText(curMenu.x + 3, curMenu.y, curMenu.textAddress, curMenu.width, FRAME(WHITETEXT));
        drawText(curMenu.x + 3, curMenu.y + 10, Fainted, 45, FRAME(WHITETEXT));
        break;
    }
    case SWITCH: {
        drawText(curMenu.x + 3, curMenu.y, curMenu.textAddress, curMenu.width, FRAME(WHITETEXT));
        drawText(curMenu.x + 3, curMenu.y + 10, SwitchIn, 90, FRAME(WHITETEXT));
        break;
    }
    case WIN: {
        drawText(curMenu.x + 3, curMenu.y + 10, win, 45, FRAME(WHITETEXT));
        break;
    }
    case LOSS: {
        drawText(curMenu.x + 3, curMenu.y, curMenu.textAddress, curMenu.width, FRAME(WHITETEXT));
        drawText(curMenu.x + 3, curMenu.y + 10, lose, 60, FRAME(WHITETEXT));
        break;
    }
    case ESCAPE_ENCOUNTER: {
        drawText(curMenu.x + 3, curMenu.y, escape, 40, FRAME(WHITETEXT));
        break;
    }
    case GATHERING: {
        // font.setCursor(curMenu.x + 3, curMenu.y + 3);
        // ////printString(font, "Gathering is not", curMenu.x + 3, curMenu.y + 3);
        // font.setCursor(curMenu.x + 3, curMenu.y + 13);
        // ////printString(font, "implemented yet", curMenu.x + 3, curMenu.y + 13);
        break;
    }
    case TEAM_CHANGE: {
        drawText(curMenu.x + 3, curMenu.y, changedIn, 105, FRAME(WHITETEXT));
        break;
    }
    case EFFECTIVENESS: {
        Modifier mod = Modifier(curMenu.textAddress);
        switch (mod) {
        case Modifier::Quarter:
            drawText(curMenu.x + 3, curMenu.y, quarter, 95, FRAME(WHITETEXT));
            break;
        case Modifier::Half:
            drawText(curMenu.x + 3, curMenu.y, half, 70, FRAME(WHITETEXT));
            break;
        case Modifier::Double:
            drawText(curMenu.x + 3, curMenu.y, doubled, 75, FRAME(WHITETEXT));
            break;
        case Modifier::Quadruple:
            drawText(curMenu.x + 3, curMenu.y, quad, 95, FRAME(WHITETEXT));
            break;
        }
        drawText(curMenu.x + 3, curMenu.y + 10, damageText, 70, FRAME(WHITETEXT));
        break;
    }
    case PLAYER_EFFECT: {
        drawText(curMenu.x + 3, curMenu.y, curMenu.textAddress, curMenu.width, FRAME(WHITETEXT));
        drawText(curMenu.x + 3, curMenu.y + 10, curMenu.detailAddress, curMenu.height, FRAME(WHITETEXT));
        break;
    }
    case ENEMY_EFFECT: {
        drawText(curMenu.x + 3, curMenu.y, curMenu.textAddress, curMenu.width, FRAME(WHITETEXT));
        drawText(curMenu.x + 3, curMenu.y + 10, curMenu.detailAddress, curMenu.height, FRAME(WHITETEXT));
        break;
    }
    default:
        break;
    }
}

void DialogMenu::pushAnimation() {
    if (!peek()) {
        return;
    }
    switch (head().type) {
    case NAME:
        animator.push(Animation{60, 0, 8, head().animation});
        break;
    case ENEMY_NAME:
        animator.push(Animation{40, 0, 8, head().animation});
        break;
    }
}
