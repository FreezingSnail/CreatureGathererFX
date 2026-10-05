#pragma once
#include "../../lib/MenuData.hpp"
#include "../world/Event.hpp"
#include "../../lib/DataTypes.hpp"

class DialogMenu {
  public:
    PopUpDialog popDialogStack[6];
    uint8_t dialogCount = 0;
    uint8_t scriptText[36] = {};

    bool peek();
    PopUpDialog &head();
    void drawPopMenu();
    bool push(PopUpDialog info);
    void clear();
    void pushMenu(PopUpDialog menuInfo);
    void pushEvent(Event event);
    void popMenu();
    void prepareHead();
};
