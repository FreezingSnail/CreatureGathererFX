#pragma once
#include "DialogMenu.hpp"
#include "../../fxdata/fxdata.h"
#include "../world/Event.hpp"
#include <ArduboyFX.h>

void drawRect(Arduboy2 *ardu, PopUpDialog *popMenu) {
    ardu->fillRect(popMenu->x, popMenu->y, popMenu->width, popMenu->height, WHITE);
    ardu->drawRect(popMenu->x + 1, popMenu->y + 1, popMenu->width - 2, popMenu->height - 2, BLACK);
}

void DialogMenu::init(Arduboy2 *Arduboy2) { this->arduboy = arduboy; }

bool __attribute__((optimize("-O0"))) DialogMenu::drawPopMenu() {
    if (this->dialogPointer < 0) {
        return false;
    }
    PopUpDialog curMenu = this->popDialogStack[this->dialogPointer];
    drawRect(this->arduboy, &curMenu);
    FX::setCursor(curMenu.x + 3, curMenu.y + 3);
    FX::drawString(curMenu.textAddress);
    return true;
}

void DialogMenu::pushMenu(PopUpDialog info) {
    if (this->dialogPointer == 3) {
        return;
    }

    this->dialogPointer++;
    this->popDialogStack[this->dialogPointer] = info;
}

void __attribute__((optimize("-O0"))) DialogMenu::pushEvent(Event event) {
    if (this->dialogPointer == 3) {
        return;
    }

    this->dialogPointer++;
    this->popDialogStack[this->dialogPointer] = PopUpDialog{0, 34, 120, 30, event.textAddress};
}

void DialogMenu::popMenu() {
    if (this->dialogPointer == -1) {
        return;
    }
    this->dialogPointer--;
}