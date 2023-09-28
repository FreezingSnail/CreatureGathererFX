#pragma once
#include "MenuV2.hpp"
#include "../../fxdata/fxdata.h"
#include "../world/Event.hpp"
#include <ArduboyFX.h>

void drawRect(Arduboy2 *ardu, PopUpMenu *popMenu) {
    ardu->fillRect(popMenu->x, popMenu->y, popMenu->width, popMenu->height, WHITE);
    ardu->drawRect(popMenu->x + 1, popMenu->y + 1, popMenu->width - 2, popMenu->height - 2, BLACK);
}

MenuV2::MenuV2() {}

MenuV2::MenuV2(Arduboy2 *Arduboy2) { this->arduboy = arduboy; }

void __attribute__((optimize("-O0"))) MenuV2::drawPopMenu() {
    if (this->head < 0) {
        return;
    }
    PopUpMenu curMenu = this->popMenuStack[this->head];
    drawRect(this->arduboy, &curMenu);
    FX::setCursor(curMenu.x + 3, curMenu.y + 3);
    FX::drawString(curMenu.textAddress);
}

void MenuV2::pushMenu(PopUpMenu info) {
    if (this->head == 3) {
        return;
    }

    this->head++;
    this->popMenuStack[this->head] = info;
}

void __attribute__((optimize("-O0"))) MenuV2::pushEvent(Event event) {
    if (this->head == 3) {
        return;
    }

    this->head++;
    this->popMenuStack[this->head] = PopUpMenu{0, 34, 120, 30, event.event.textAddress};
}

void MenuV2::popMenu() {
    if (this->head == -1) {
        return;
    }
    this->head--;
}