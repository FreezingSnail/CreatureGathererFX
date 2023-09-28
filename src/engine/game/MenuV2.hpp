#pragma once
#include "../world/Event.hpp"
#include <ArduboyFX.h>

struct PopUpMenu {
    uint8_t x, y, width, height;
    uint24_t textAddress;
};

class MenuV2 {
  public:
    Arduboy2 *arduboy;
    PopUpMenu popMenuStack[4];
    int8_t head = -1;

    MenuV2();
    MenuV2(Arduboy2 *arduboy);
    void drawPopMenu();
    void pushMenu(PopUpMenu menuInfo);
    void pushEvent(Event event);
    void popMenu();
};