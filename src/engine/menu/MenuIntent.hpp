#pragma once

#include <stdint.h>

enum class MenuIntentKind : uint8_t {
    None,
    SelectMove,
    SelectParty,
    Gather,
    Escape,
    Back,
    Pass,
    SelectItem,
};

struct MenuIntent {
    MenuIntentKind kind;
    uint8_t index;
};
