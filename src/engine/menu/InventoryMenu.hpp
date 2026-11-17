#pragma once

#include <stdint.h>

#include "../../item/Inventory.hpp"
#include "../../lib/ListView.hpp"
#include "../../lib/uint24.h"
#include "MenuIntent.hpp"

// A four-row bag page: the ListView window plus the cached name addresses for
// exactly those rows. Names are resolved on open and scroll only; draw reads
// the cache. The world overlay instance lives in file statics, not ModeState.
struct InventoryMenu {
    ListView view;
    uint8_t rowId[4];
    uint8_t rowKind[4];
    uint24_t rowName[4][2];
    uint8_t rowParts[4];
    // Battle Heal-only page filter: one bit per consumable id. Built on open.
    uint8_t healMask;
};

// AVR layout: 4 ListView + 4 ids + 4 kinds + 24 address bytes + 4 parts + filter.
#ifdef __AVR__
static_assert(sizeof(InventoryMenu) == 41, "inventory menu cache must remain 41 bytes");
#else
static_assert(sizeof(InventoryMenu) == 52, "host uint24_t padding must remain understood");
#endif

void inventoryMenuOpen(InventoryMenu &m, const item::Inventory &inv);
void inventoryMenuMove(InventoryMenu &m, const item::Inventory &inv, int8_t delta);
// Visible-row intent: index is the row inside the window, so the caller reads
// rowKind[index]/rowId[index] and mixed lure/consumable rows never alias.
// Returns {None, 0} while the bag is empty.
MenuIntent inventoryMenuSelect(const InventoryMenu &m);
void drawInventoryMenu(const InventoryMenu &m, const item::Inventory &inv);

// Battle page: only consumables whose authored effect is Heal are listed, with
// non-Heal rows skipped so a mixed bag never aliases a visible row. Open and
// scroll build/resolve the cached rows (transition reads); draw never does.
void inventoryMenuOpenHeal(InventoryMenu &m, const item::Inventory &inv);
void inventoryMenuMoveHeal(InventoryMenu &m, const item::Inventory &inv, int8_t delta);
MenuIntent inventoryMenuSelectHeal(const InventoryMenu &m);

// --- World bag overlay ----------------------------------------------------
// Fresh-edge bits match the world picker's edge encoding (up, down, A, B) so
// WorldEngine passes one sampled edge set to either overlay.
constexpr uint8_t INVENTORY_MENU_UP = 1u << 0;
constexpr uint8_t INVENTORY_MENU_DOWN = 1u << 1;
constexpr uint8_t INVENTORY_MENU_A = 1u << 2;
constexpr uint8_t INVENTORY_MENU_B = 1u << 3;

// Frozen-world bag page. A fresh B opens only while the world is idle; A
// selects without ever taking an item; B closes. Returns true while the overlay
// owns the world frame so the caller must not step or interact.
bool inventoryMenuWorldInput(uint8_t edges, bool worldIdle, const item::Inventory &inv);
bool inventoryMenuWorldOpen();
void inventoryMenuWorldDraw(const item::Inventory &inv);
