#pragma once

#include <stdint.h>

#include "ItemIds.hpp"

// Key-item ids are their own namespace. They never index the generated
// src/flags/flags.hpp ids or FLAG_BIT_ARRAY, whose script-allocated ids can move.
namespace item {

struct KeyItems {
    uint8_t bits[(KEY_ITEM_COUNT + 7) / 8];
};

static_assert(sizeof(KeyItems) == 1, "key-item unlocked storage is part of the save format");

void keyItemsClear(KeyItems &k);
bool keyItemUnlocked(const KeyItems &k, uint8_t id);
void keyItemUnlock(KeyItems &k, uint8_t id);

}  // namespace item
