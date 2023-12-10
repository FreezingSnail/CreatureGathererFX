#pragma once
#include "DialogMenu.hpp"
#include "../../fxdata/fxdata.h"
#include "../../lib/Type.hpp"
#include "../world/Event.hpp"
#include <ArduboyFX.h>
#define dbf __attribute__((optimize("-O0")))

void drawRect(Arduboy2 *ardu, PopUpDialog *popMenu) {
    ardu->fillRect(popMenu->x, popMenu->y, popMenu->width, popMenu->height, WHITE);
    ardu->drawRect(popMenu->x + 1, popMenu->y + 1, popMenu->width - 2, popMenu->height - 2, BLACK);
}

void DialogMenu::init(Arduboy2 *Arduboy2) { this->arduboy = arduboy; }

void DialogMenu::drawPopMenu() {
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
    case ESCAPE_ENCOUNTER:
        FX::setCursor(curMenu.x + 3, curMenu.y + 3);
        FX::drawString("You escaped");
        break;
    case GATHERING:
        FX::setCursor(curMenu.x + 3, curMenu.y + 3);
        FX::drawString("Gathering is not");
        FX::setCursor(curMenu.x + 3, curMenu.y + 13);
        FX::drawString("implemented yet");
        break;
    case TEAM_CHANGE:
        FX::setCursor(curMenu.x + 3, curMenu.y + 3);
        FX::drawString("You Change creature");
        break;

    case EFFECTIVENESS:
        Modifier mod = Modifier(curMenu.textAddress);
        FX::setCursor(curMenu.x + 3, curMenu.y + 3);

        switch (mod) {
        case Modifier::Quarter:
            FX::drawString("It's barely damaging");
            break;
        case Modifier::Half:
            FX::drawString("It does some");
            FX::setCursor(curMenu.x + 3, curMenu.y + 13);
            FX::drawString("damage");
            break;
        case Modifier::Double:
            FX::drawString("It does great");
            FX::setCursor(curMenu.x + 3, curMenu.y + 13);
            FX::drawString("damage");
            break;
        case Modifier::Quadruple:
            FX::drawString("It's devistating");
            FX::setCursor(curMenu.x + 3, curMenu.y + 13);
            FX::drawString("damage");
            break;
        }
        break;

    default:
        FX::setCursor(curMenu.x + 3, curMenu.y + 3);
        FX::drawString("default");
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