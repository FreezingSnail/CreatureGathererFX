#pragma once
#include "DialogMenu.hpp"
#include "../../fxdata/fxdata.h"
#include "../../lib/Type.hpp"
#include "../draw.h"
#include "../world/Event.hpp"
#include <ArduboyFX.h>
#define dbf __attribute__((optimize("-O0")))

void drawRect(Arduboy2 *ardu, PopUpDialog *popMenu) {
    // ardu->fillRect(popMenu->x, popMenu->y, popMenu->width, popMenu->height, WHITE);
    // ardu->drawRect(popMenu->x + 1, popMenu->y + 1, popMenu->width - 2, popMenu->height - 2, BLACK);
}

void DialogMenu::init(Arduboy2 *Arduboy2) { this->arduboy = arduboy; }

void DialogMenu::drawPopMenu() {
    PopUpDialog curMenu = popDialogStack[0];
    FX::drawBitmap(0, 43, battleMenu, 0, dbmNormal);

    uint24_t addr;
    setTextColorBlack();
    switch (curMenu.type) {
    case TEXT:
        FX::setCursor(curMenu.x + 3, curMenu.y + 13);
        FX::drawString(curMenu.textAddress);

        break;
    case DAMAGE:
    case ENEMY_DAMAGE:
        FX::setCursor(curMenu.x + 3, curMenu.y + 3);
        FX::drawNumber(curMenu.damage);
        FX::setCursor(curMenu.x + 20, curMenu.y + 3);
        FX::drawString(MenuFXData::damageText);
        break;

    case NAME:
        FX::setCursor(curMenu.x + 3, curMenu.y + 3);
        addr = FX::readIndexedUInt24(CreatureData::creatureNames, curMenu.textAddress);
        FX::drawString(addr);
        FX::setCursor(curMenu.x + 3, curMenu.y + 13);
        FX::drawString(MenuFXData::attackText);
        break;
    case ENEMY_NAME:
        FX::setCursor(curMenu.x + 3, curMenu.y + 3);
        addr = FX::readIndexedUInt24(CreatureData::creatureNames, curMenu.textAddress);
        FX::drawString(addr);
        FX::setCursor(curMenu.x + 3, curMenu.y + 13);
        FX::drawString(MenuFXData::enemyAttackText);
        break;
    case FAINT:
        FX::setCursor(curMenu.x + 3, curMenu.y + 3);
        addr = FX::readIndexedUInt24(CreatureData::creatureNames, curMenu.textAddress);
        FX::drawString(addr);
        FX::setCursor(curMenu.x + 3, curMenu.y + 13);
        FX::drawString(MenuFXData::Fainted);
        break;
    case SWITCH:
        FX::setCursor(curMenu.x + 3, curMenu.y + 3);
        addr = FX::readIndexedUInt24(CreatureData::creatureNames, curMenu.textAddress);
        FX::drawString(addr);
        FX::setCursor(curMenu.x + 3, curMenu.y + 13);
        FX::drawString(MenuFXData::SwitchIn);
        break;
    case WIN:
        FX::setCursor(curMenu.x + 3, curMenu.y + 3);
        FX::drawString(MenuFXData::win);
        break;
    case LOSS:
        FX::setCursor(curMenu.x + 3, curMenu.y + 3);
        addr = FX::readIndexedUInt24(CreatureData::creatureNames, curMenu.textAddress);
        FX::drawString(addr);
        FX::setCursor(curMenu.x + 3, curMenu.y + 13);
        FX::drawString(MenuFXData::lose);
        break;
    case ESCAPE_ENCOUNTER:
        FX::setCursor(curMenu.x + 3, curMenu.y + 3);
        FX::drawString(MenuFXData::escaped);
        break;
    case GATHERING:
        FX::setCursor(curMenu.x + 3, curMenu.y + 3);
        FX::drawString("Gathering is not");
        FX::setCursor(curMenu.x + 3, curMenu.y + 13);
        FX::drawString("implemented yet");
        break;
    case TEAM_CHANGE:
        FX::setCursor(curMenu.x + 3, curMenu.y + 3);
        FX::drawString(MenuFXData::changedIn);
        break;

    case EFFECTIVENESS:
        Modifier mod = Modifier(curMenu.textAddress);
        FX::setCursor(curMenu.x + 3, curMenu.y + 3);

        switch (mod) {
        case Modifier::Quarter:
            FX::drawString(MenuFXData::quarter);
            break;
        case Modifier::Half:
            FX::drawString(MenuFXData::half);
            FX::setCursor(curMenu.x + 3, curMenu.y + 13);
            FX::drawString(MenuFXData::damage);
            break;
        case Modifier::Double:
            FX::drawString(MenuFXData::doubled);
            FX::setCursor(curMenu.x + 3, curMenu.y + 13);
            FX::drawString(MenuFXData::damage);
            break;
        case Modifier::Quadruple:
            FX::drawString(MenuFXData::quad);
            FX::setCursor(curMenu.x + 3, curMenu.y + 13);
            FX::drawString(MenuFXData::damage);
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