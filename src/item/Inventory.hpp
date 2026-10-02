#pragma once

#include <stdint.h>

#include "ItemIds.hpp"

namespace item {

struct Inventory {
    uint8_t lures[LURE_COUNT];
    uint8_t consumables[CONSUMABLE_COUNT];
};

static_assert(sizeof(Inventory) == 32, "inventory count storage is part of the save format");

void inventoryClear(Inventory &inv);
uint8_t inventoryCount(const Inventory &inv, ItemKind kind, uint8_t id);
uint8_t inventoryAdd(Inventory &inv, ItemKind kind, uint8_t id, uint8_t n);
bool inventoryTake(Inventory &inv, ItemKind kind, uint8_t id, uint8_t n);
uint8_t inventoryNonZeroCount(const Inventory &inv);
// O(32), intended for menu open and scrolling, never per frame.
bool inventoryNthNonZero(const Inventory &inv, uint8_t n, ItemKind &kindOut, uint8_t &idOut);

}  // namespace item
