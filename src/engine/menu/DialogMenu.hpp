#pragma once
#include "../../lib/MenuData.hpp"
#include "../world/Event.hpp"
#include <ArduboyFX.h>

struct PopUpDialog {
    uint8_t x, y, width, height;
    uint24_t textAddress;
    uint16_t damage;
    DialogType type;
    uint24_t animation;
};

class DialogMenu {
  public:
    PopUpDialog popDialogStack[6];
    int8_t dialogPointer = -1;

    bool peek();
    void drawPopMenu();
    void pushMenu(PopUpDialog menuInfo);
    void pushEvent(Event event);
    void popMenu();
    void pushAnimation();
};