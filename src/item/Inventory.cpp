#include "Inventory.hpp"

namespace item {
namespace {

Inventory *activeBattleInventory = nullptr;

uint8_t *countFor(Inventory &inv, ItemKind kind, uint8_t id) {
    switch (kind) {
        case ItemKind::Lure:
            return id < LURE_COUNT ? &inv.lures[id] : nullptr;
        case ItemKind::Consumable:
            return id < CONSUMABLE_COUNT ? &inv.consumables[id] : nullptr;
        case ItemKind::Key:
            return nullptr;
    }
    return nullptr;
}

const uint8_t *countFor(const Inventory &inv, ItemKind kind, uint8_t id) {
    switch (kind) {
        case ItemKind::Lure:
            return id < LURE_COUNT ? &inv.lures[id] : nullptr;
        case ItemKind::Consumable:
            return id < CONSUMABLE_COUNT ? &inv.consumables[id] : nullptr;
        case ItemKind::Key:
            return nullptr;
    }
    return nullptr;
}

}  // namespace

void setBattleInventory(Inventory *inventory) {
    activeBattleInventory = inventory;
}

Inventory *battleInventory() {
    return activeBattleInventory;
}

void inventoryClear(Inventory &inv) {
    for (uint8_t i = 0; i < LURE_COUNT; ++i) inv.lures[i] = 0;
    for (uint8_t i = 0; i < CONSUMABLE_COUNT; ++i) inv.consumables[i] = 0;
}

uint8_t inventoryCount(const Inventory &inv, ItemKind kind, uint8_t id) {
    const uint8_t *count = countFor(inv, kind, id);
    return count ? *count : 0;
}

// Saturates at COUNT_MAX and reports the amount actually stored.
uint8_t inventoryAdd(Inventory &inv, ItemKind kind, uint8_t id, uint8_t n) {
    uint8_t *count = countFor(inv, kind, id);
    if (!count) return 0;
    const uint8_t room = COUNT_MAX - *count;
    const uint8_t added = n < room ? n : room;
    *count += added;
    return added;
}

bool inventoryTake(Inventory &inv, ItemKind kind, uint8_t id, uint8_t n) {
    uint8_t *count = countFor(inv, kind, id);
    if (!count || *count < n) return false;
    *count -= n;
    return true;
}

uint8_t inventoryNonZeroCount(const Inventory &inv) {
    uint8_t total = 0;
    for (uint8_t i = 0; i < LURE_COUNT; ++i) total += inv.lures[i] != 0;
    for (uint8_t i = 0; i < CONSUMABLE_COUNT; ++i) total += inv.consumables[i] != 0;
    return total;
}

bool inventoryNthNonZero(const Inventory &inv, uint8_t n, ItemKind &kindOut, uint8_t &idOut) {
    // Screen list order: nonzero lures by ascending id, then consumables by ascending id.
    for (uint8_t i = 0; i < LURE_COUNT; ++i) {
        if (inv.lures[i] && n-- == 0) {
            kindOut = ItemKind::Lure;
            idOut = i;
            return true;
        }
    }
    for (uint8_t i = 0; i < CONSUMABLE_COUNT; ++i) {
        if (inv.consumables[i] && n-- == 0) {
            kindOut = ItemKind::Consumable;
            idOut = i;
            return true;
        }
    }
    return false;
}

}  // namespace item
