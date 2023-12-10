#pragma once
#include "../../Globals.hpp"
#include "../../lib/MenuData.hpp"
#include "../world/Event.hpp"
#include <ArduboyFX.h>

struct PopUpDialog {
    uint8_t x, y, width, height;
    uint24_t textAddress;
    uint16_t damage;
    DialogType type;
};

class DialogMenu {
  public:
    Arduboy2 *arduboy;
    PopUpDialog popDialogStack[6];
    int8_t dialogPointer = -1;

    void init(Arduboy2 *arduboy);
    bool peek();
    void drawPopMenu();
    void pushMenu(PopUpDialog menuInfo);
    void pushEvent(Event event);
    void popMenu();
};