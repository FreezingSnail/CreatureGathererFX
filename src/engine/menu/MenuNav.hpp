#pragma once

#include <stdint.h>

#include "../../lib/MenuStack.hpp"

struct MenuDesc {
    uint8_t itemCount;
    uint8_t cols;
    uint8_t wrap;
};

enum MenuNavButton : uint8_t {
    MENU_NAV_LEFT = static_cast<uint8_t>(1u << 5),
    MENU_NAV_RIGHT = static_cast<uint8_t>(1u << 6),
    MENU_NAV_UP = static_cast<uint8_t>(1u << 7),
    MENU_NAV_DOWN = static_cast<uint8_t>(1u << 4),
};

const MenuDesc &menuDescFor(MenuEnum menu);
uint8_t menuNavMove(const MenuDesc &desc, uint8_t cursor, uint8_t buttons);
