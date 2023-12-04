#pragma once
#include "DialogMenu.hpp"
#include "../../fxdata/fxdata.h"
#include "../world/Event.hpp"
#include <ArduboyFX.h>
#define dbf __attribute__((optimize("-O0")))

void drawRect(Arduboy2 *ardu, PopUpDialog *popMenu) {
    ardu->fillRect(popMenu->x, popMenu->y, popMenu->width, popMenu->height, WHITE);
    ardu->drawRect(popMenu->x + 1, popMenu->y + 1, popMenu->width - 2, popMenu->height - 2, BLACK);
}

void DialogMenu::init(Arduboy2 *Arduboy2) { this->arduboy = arduboy; }

void dbf DialogMenu::drawPopMenu() {
    PopUpDialog curMenu = popDialogStack[0];
    drawRect(arduboy, &curMenu);
    uint24_t addr;
    switch (curMenu.type) {
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
        FX::drawString("Fainted");
        break;
    case SWITCH:
        FX::setCursor(curMenu.x + 3, curMenu.y + 3);
        addr = FX::readIndexedUInt24(CreatureData::creatureNames, curMenu.textAddress);
        FX::drawString(addr);
        FX::setCursor(curMenu.x + 3, curMenu.y + 13);
        FX::drawString("enters the field");
        break;
    case WIN:
        FX::setCursor(curMenu.x + 3, curMenu.y + 3);
        FX::drawString("You win");
        break;
    case LOSS:
        FX::setCursor(curMenu.x + 3, curMenu.y + 3);
        addr = FX::readIndexedUInt24(CreatureData::creatureNames, curMenu.textAddress);
        FX::drawString(addr);
        FX::setCursor(curMenu.x + 3, curMenu.y + 13);
        FX::drawString("You loose");
        break;

    default:
        FX::setCursor(curMenu.x + 3, curMenu.y + 3);
        FX::drawString(curMenu.textAddress);
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
    popDialogStack[dialogPointer] = PopUpDialog{0, 34, 120, 30, event.textAddress};
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