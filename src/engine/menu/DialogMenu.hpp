#pragma once
#include "../../lib/MenuData.hpp"
#include "../world/Event.hpp"
#include <ArduboyFX.h>

struct PopUpDialog {
    uint8_t x, y, width, height;
    uint24_t textAddress;
};

class DialogMenu {
  public:
    Arduboy2 *arduboy;
    PopUpDialog popDialogStack[4];
    int8_t dialogPointer = -1;

    void init(Arduboy2 *arduboy);
    bool drawPopMenu();
    void pushMenu(PopUpDialog menuInfo);
    void pushEvent(Event event);
    void popMenu();
};