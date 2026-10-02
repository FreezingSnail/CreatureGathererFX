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
} // namespace

void DialogMenu::drawPopMenu() {
    PopUpDialog curMenu = head();
    SpritesU::drawOverwriteFX(0, 40, 128, 24, battleMenu - 2, FRAME(0));

    setTextColorBlack();

    switch (curMenu.type) {
    case TEXT: {
        // Event::load supplies a direct address in the separate raw ASCII event format.
        SpritesU::drawOverwriteFX(curMenu.x + 8, curMenu.y + 2, curMenu.textAddress, FRAME(WHITETEXT));
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
