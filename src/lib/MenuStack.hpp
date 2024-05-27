#pragma once
#include <stdint.h>

enum MenuEnum {
    BATTLE_MOVE_SELECT,
    BATTLE_CREATURE_SELECT,
    BATTLE_OPTIONS,
    WORLD_OPTIONS,
};

struct MenuStack {
    MenuEnum stack[6];
    int8_t pointer = -1;

    void push(MenuEnum menu);
    void pop();
    MenuEnum top();
};