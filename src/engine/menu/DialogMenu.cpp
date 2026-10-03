#pragma once
#include "DialogMenu.hpp"
#include "../../external/ArduboyG.h"
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
void drawDialogString(int16_t x, int16_t y, uint24_t address, uint8_t width) {
    if (width != 0) {
        // String sprite symbols point at raw bitmap bytes. The explicit-size
        // overload advances two bytes, so compensate just as drawNumbersBlack does.
        SpritesU::drawOverwriteFX(x, y, width, 8, address - 2, FRAME(WHITETEXT));
    }
}

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
            SpritesU::drawOverwriteFX(x + column * 6, y + line * 8,
                                      5, 6, fontTrimmed - 2, FRAME(glyph));
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
    SpritesU::drawOverwriteFX(0, 40, 128, 24, battleMenu - 2, FRAME(0));

    setTextColorBlack();

    switch (curMenu.type) {
    case TEXT: {
        // Event::load supplies a direct address in the separate raw ASCII event format.
        SpritesU::drawOverwriteFX(curMenu.x + 8, curMenu.y + 2,
                                  curMenu.width, curMenu.height, curMenu.textAddress, FRAME(WHITETEXT));
        break;
    }
    case SCRIPT_TEXT: {
        drawScriptText(scriptText, curMenu.width, curMenu.x + 8, curMenu.y + 2);
        break;
    }
    case DAMAGE: {
        drawDialogString(curMenu.x + 12, curMenu.y + 2, damageText, 70);
        drawNumbersBlack(curMenu.x + 4, curMenu.y + 3, curMenu.damage);
        break;
    }
    case ENEMY_DAMAGE: {
        // font.setCursor(curMenu.x + 3, curMenu.y + 3);
        // font.println(curMenu.damage);
        drawDialogString(curMenu.x + 12, curMenu.y + 2, damageText, 70);
        drawNumbersBlack(curMenu.x + 4, curMenu.y + 3, curMenu.damage);
        break;
    }
    case NAME: {
        drawDialogString(curMenu.x + 3, curMenu.y + 2, curMenu.textAddress, curMenu.width);
        drawDialogString(curMenu.x + 3, curMenu.y + 10, attackText, 90);
        if (curMenu.damage != 0) {
            drawDialogString(curMenu.x + 83, curMenu.y + 10, curMenu.detailAddress, curMenu.height);
        }
        break;
    }
    case ENEMY_NAME: {
        drawDialogString(curMenu.x + 3, curMenu.y, curMenu.textAddress, curMenu.width);
        drawDialogString(curMenu.x + 3, curMenu.y + 10, enemyAttackText, 70);
        if (curMenu.damage != 0) {
            drawDialogString(curMenu.x + 83, curMenu.y + 10, curMenu.detailAddress, curMenu.height);
        }
        break;
    }
    case FAINT: {
        drawDialogString(curMenu.x + 3, curMenu.y, curMenu.textAddress, curMenu.width);
        drawDialogString(curMenu.x + 3, curMenu.y + 10, Fainted, 45);
        break;
    }
    case SWITCH: {
        drawDialogString(curMenu.x + 3, curMenu.y, curMenu.textAddress, curMenu.width);
        drawDialogString(curMenu.x + 3, curMenu.y + 10, SwitchIn, 90);
        break;
    }
    case WIN: {
        drawDialogString(curMenu.x + 3, curMenu.y + 10, win, 45);
        break;
    }
    case LOSS: {
        drawDialogString(curMenu.x + 3, curMenu.y, curMenu.textAddress, curMenu.width);
        drawDialogString(curMenu.x + 3, curMenu.y + 10, lose, 60);
        break;
    }
    case ESCAPE_ENCOUNTER: {
        drawDialogString(curMenu.x + 3, curMenu.y, escape, 40);
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
        drawDialogString(curMenu.x + 3, curMenu.y, changedIn, 105);
        break;
    }
    case EFFECTIVENESS: {
        Modifier mod = Modifier(curMenu.textAddress);
        switch (mod) {
        case Modifier::Quarter:
            drawDialogString(curMenu.x + 3, curMenu.y, quarter, 95);
            break;
        case Modifier::Half:
            drawDialogString(curMenu.x + 3, curMenu.y, half, 70);
            break;
        case Modifier::Double:
            drawDialogString(curMenu.x + 3, curMenu.y, doubled, 75);
            break;
        case Modifier::Quadruple:
            drawDialogString(curMenu.x + 3, curMenu.y, quad, 95);
            break;
        }
        drawDialogString(curMenu.x + 3, curMenu.y + 10, damageText, 70);
        break;
    }
    case PLAYER_EFFECT: {
        drawDialogString(curMenu.x + 3, curMenu.y, curMenu.textAddress, curMenu.width);
        drawDialogString(curMenu.x + 3, curMenu.y + 10, curMenu.detailAddress, curMenu.height);
        break;
    }
    case ENEMY_EFFECT: {
        drawDialogString(curMenu.x + 3, curMenu.y, curMenu.textAddress, curMenu.width);
        drawDialogString(curMenu.x + 3, curMenu.y + 10, curMenu.detailAddress, curMenu.height);
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
