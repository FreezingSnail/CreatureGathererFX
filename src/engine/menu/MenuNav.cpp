#include "MenuNav.hpp"

namespace {
constexpr MenuDesc MENU_DESCS[] = {
    {4, 2, 1},  // BATTLE_MOVE_SELECT
    {4, 2, 1},  // BATTLE_CREATURE_SELECT
    {4, 2, 1},  // BATTLE_OPTIONS
    {4, 2, 1},  // WORLD_OPTIONS
    {31, 1, 0}, // ARENA_MENU
};

static_assert(static_cast<uint8_t>(ARENA_MENU) + 1u ==
                  sizeof(MENU_DESCS) / sizeof(MENU_DESCS[0]),
              "menu descriptor table must cover every MenuEnum value");
}

const MenuDesc &menuDescFor(MenuEnum menu) {
    const uint8_t index = static_cast<uint8_t>(menu);
    if (index > static_cast<uint8_t>(ARENA_MENU)) {
        return MENU_DESCS[static_cast<uint8_t>(BATTLE_OPTIONS)];
    }
    return MENU_DESCS[index];
}

uint8_t menuNavMove(const MenuDesc &desc, uint8_t cursor, uint8_t buttons) {
    if (desc.itemCount == 0) return 0;

    int16_t next = cursor;
    if (buttons & MENU_NAV_LEFT) --next;
    if (buttons & MENU_NAV_RIGHT) ++next;
    if (buttons & MENU_NAV_DOWN) next += desc.cols;
    if (buttons & MENU_NAV_UP) next -= desc.cols;

    if (desc.wrap) {
        if (next >= desc.itemCount) return 0;
        if (next < 0) return static_cast<uint8_t>(desc.itemCount - 1);
    } else {
        if (next < 0) return 0;
        if (next >= desc.itemCount) return static_cast<uint8_t>(desc.itemCount - 1);
    }
    return static_cast<uint8_t>(next);
}
