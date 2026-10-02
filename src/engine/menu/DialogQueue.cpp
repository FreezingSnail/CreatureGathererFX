#include "DialogMenu.hpp"

bool DialogMenu::peek() {
    return dialogCount != 0;
}

PopUpDialog &DialogMenu::head() {
    return popDialogStack[0];
}

bool DialogMenu::push(PopUpDialog info) {
    if (dialogCount >= 6) {
        return false;
    }
    popDialogStack[dialogCount++] = info;
    return true;
}

void DialogMenu::clear() {
    for (uint8_t i = 0; i < 6; ++i) {
        popDialogStack[i] = PopUpDialog{0, 0, 0, 0, 0, 0, TEXT, 0};
    }
    dialogCount = 0;
}

void DialogMenu::pushMenu(PopUpDialog info) {
    push(info);
}

void DialogMenu::pushEvent(Event event) {
    PopUpDialog info{0, 34, 120, 30, event.textAddress, 0, TEXT, 0};
    push(info);
}

void DialogMenu::popMenu() {
    if (dialogCount == 0) {
        return;
    }
    for (uint8_t i = 0; i + 1 < dialogCount; ++i) {
        popDialogStack[i] = popDialogStack[i + 1];
    }
    --dialogCount;
    popDialogStack[dialogCount] = PopUpDialog{0, 0, 0, 0, 0, 0, TEXT, 0};
    if (peek() && head().animation != 0) {
        pushAnimation();
    }
}
