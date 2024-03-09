#pragma once
#include "DialogMenu.hpp"
#include "../../external/ArduboyG.h"
#include "../../fxdata.h"
#include "../../lib/Type.hpp"
#include "../draw.h"
#include "../world/Event.hpp"
#include <ArduboyFX.h>
#include "../../lib/Text.hpp"
#include "../../common.hpp"

#define WHITETEXT 1
#define BLACKTEXT 0

void drawRect(PopUpDialog *popMenu) {
    // ardu->fillRect(popMenu->x, popMenu->y, popMenu->width, popMenu->height, WHITE);
    // ardu->drawRect(popMenu->x + 1, popMenu->y + 1, popMenu->width - 2, popMenu->height - 2, BLACK);
}

void DialogMenu::drawPopMenu() {

    PopUpDialog curMenu = popDialogStack[0];
    SpritesU::drawOverwriteFX(0, 40, battleMenu, FRAME(0));
    //  SpritesU::fillRect(0, 42, 128, 32, WHITE);

    uint24_t addr;
    setTextColorBlack();
    switch (curMenu.type) {
    case TEXT:
        // font.setCursor(curMenu.x + 3, curMenu.y + 13);
        // printString(font, curMenu.textAddress, curMenu.x + 3, curMenu.y + 13);

        break;
    case DAMAGE:
    case ENEMY_DAMAGE:
        font.setCursor(curMenu.x + 3, curMenu.y + 3);
        font.println(curMenu.damage);
        // printString(font, MenuFXData::damageText, curMenu.x + 20, curMenu.y + 3);
        break;

    case NAME:
        // font.setCursor(curMenu.x + 3, curMenu.y + 3);
        addr = FX::readIndexedUInt24(CreatureNames::CreatureNames, curMenu.textAddress);
        // //printString(font, addr, curMenu.x + 3, curMenu.y + 3);
        SpritesU::drawOverwriteFX(curMenu.x + 3, curMenu.y + 2, addr, FRAME(WHITETEXT));
        // font.setCursor(curMenu.x + 3, curMenu.y + 13);
        // printString(font, attackText, curMenu.x + 3, curMenu.y + 7);
        break;
    case ENEMY_NAME:
        // font.setCursor(curMenu.x + 3, curMenu.y + 3);
        // addr = FX::readIndexedUInt24(CreatureData::creatureNames, curMenu.textAddress);
        // //printString(font, addr, curMenu.x + 3, curMenu.y + 3);
        addr = FX::readIndexedUInt24(CreatureNames::CreatureNames, curMenu.textAddress);

        SpritesU::drawOverwriteFX(curMenu.x + 3, curMenu.y, addr, FRAME(WHITETEXT));
        SpritesU::drawOverwriteFX(curMenu.x + 3, curMenu.y + 10, enemyAttackText, FRAME(WHITETEXT));
        // font.setCursor(curMenu.x + 3, curMenu.y + 13);
        break;
    case FAINT:
        // font.setCursor(curMenu.x + 3, curMenu.y + 3);
        // addr = FX::readIndexedUInt24(CreatureData::creatureNames, curMenu.textAddress);
        // //printString(font, addr, curMenu.x + 3, curMenu.y + 3);
        // font.setCursor(curMenu.x + 3, curMenu.y + 13);
        SpritesU::drawOverwriteFX(curMenu.x + 3, curMenu.y + 3, CreatureNames::CreatureNames,
                                  curMenu.textAddress * 3 + arduboy.currentPlane());

        // printString(font, MenuFXData::Fainted, curMenu.x + 3, curMenu.y + 13);
        break;
    case SWITCH:
        // font.setCursor(curMenu.x + 3, curMenu.y + 3);
        // addr = FX::readIndexedUInt24(CreatureData::creatureNames, curMenu.textAddress);
        // //printString(font, addr, curMenu.x + 3, curMenu.y + 3);
        SpritesU::drawOverwriteFX(curMenu.x + 3, curMenu.y + 3, CreatureNames::CreatureNames,
                                  curMenu.textAddress * 3 + arduboy.currentPlane());

        // font.setCursor(curMenu.x + 3, curMenu.y + 13);
        // printString(font, MenuFXData::SwitchIn, curMenu.x + 3, curMenu.y + 13);
        break;
    case WIN:
        font.setCursor(curMenu.x + 3, curMenu.y + 3);
        // printString(font, MenuFXData::win, curMenu.x + 3, curMenu.y + 13);
        break;
    case LOSS:
        // font.setCursor(curMenu.x + 3, curMenu.y + 3);
        // addr = FX::readIndexedUInt24(CreatureData::creatureNames, curMenu.textAddress);
        // //printString(font, addr, curMenu.x + 3, curMenu.y + 3);
        SpritesU::drawOverwriteFX(curMenu.x + 3, curMenu.y + 3, CreatureNames::CreatureNames,
                                  curMenu.textAddress * 3 + arduboy.currentPlane());

        // font.setCursor(curMenu.x + 3, curMenu.y + 13);
        // printString(font, MenuFXData::lose, curMenu.x + 3, curMenu.y + 13);
        break;
    case ESCAPE_ENCOUNTER:
        // font.setCursor(curMenu.x + 3, curMenu.y + 3);
        // printString(font, MenuFXData::escaped, curMenu.x + 3, curMenu.y + 3);
        break;
    case GATHERING:
        // font.setCursor(curMenu.x + 3, curMenu.y + 3);
        // //printString(font, "Gathering is not", curMenu.x + 3, curMenu.y + 3);
        // font.setCursor(curMenu.x + 3, curMenu.y + 13);
        // //printString(font, "implemented yet", curMenu.x + 3, curMenu.y + 13);
        break;
    case TEAM_CHANGE:
        // font.setCursor(curMenu.x + 3, curMenu.y + 3);
        // printString(font, MenuFXData::changedIn, curMenu.x + 3, curMenu.y + 3);
        break;

    case EFFECTIVENESS:
        Modifier mod = Modifier(curMenu.textAddress);
        font.setCursor(curMenu.x + 3, curMenu.y + 3);

        switch (mod) {
        case Modifier::Quarter:
            // printString(font, MenuFXData::quarter, curMenu.x + 3, curMenu.y + 3);
            break;
        case Modifier::Half:
            // printString(font, MenuFXData::half, curMenu.x + 3, curMenu.y + 3);
            //    font.setCursor(curMenu.x + 3, curMenu.y + 13);
            // printString(font, MenuFXData::damage, curMenu.x + 3, curMenu.y + 13);
            break;
        case Modifier::Double:
            // printString(font, MenuFXData::doubled, curMenu.x + 3, curMenu.y + 3);
            //   font.setCursor(curMenu.x + 3, curMenu.y + 13);
            // printString(font, MenuFXData::damage, curMenu.x + 3, curMenu.y + 13);
            break;
        case Modifier::Quadruple:
            // printString(font, MenuFXData::quad, curMenu.x + 3, curMenu.y + 3);
            //   font.setCursor(curMenu.x + 3, curMenu.y + 13);
            // printString(font, MenuFXData::damage, curMenu.x + 3, curMenu.y + 13);
            break;
        }
        break;
    }
}

bool DialogMenu::peek() {
    if (dialogPointer < 0)
        return false;
    return true;
}

void DialogMenu::pushMenu(PopUpDialog info) {
    dialogPointer++;
    popDialogStack[dialogPointer] = info;
}

void DialogMenu::pushEvent(Event event) {
    dialogPointer++;
    popDialogStack[dialogPointer] = PopUpDialog{0, 34, 120, 30, event.textAddress, TEXT};
}

void DialogMenu::popMenu() {
    if (dialogPointer <= 0) {
        dialogPointer--;
        return;
    }
    for (uint8_t i = 0; i < dialogPointer; i++) {
        popDialogStack[i] = popDialogStack[i + 1];
    }
    dialogPointer--;
}