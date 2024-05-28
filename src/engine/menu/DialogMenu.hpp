#pragma once
#include "../../lib/MenuData.hpp"
#include "../world/Event.hpp"
#include "../../lib/DataTypes.hpp"

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