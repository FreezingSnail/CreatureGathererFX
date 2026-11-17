#include "InventoryMenu.hpp"

#include <avr/pgmspace.h>

#include "../../item/ConsumableDef.hpp"
#include "../../item/Inventory.hpp"
#include "../../item/ItemNames.hpp"
#include "../../lib/Blit.hpp"
#include "../../lib/FxReadCounter.hpp"
#include "../../lib/Text.hpp"
#include "../PpGlyph.hpp"

namespace {
constexpr uint8_t kVisibleRows = 4;
constexpr uint8_t kRowY[kVisibleRows] = {8, 21, 34, 47};

// Character counts from data/text/strings.txt. The string parser keeps one
// leading space, so every generated name bitmap is (characters + 1) * 5 wide.
// Kept in program memory: draw never resolves item metadata.
const uint8_t tierChars[] PROGMEM = {5, 4, 4};
const uint8_t typeChars[] PROGMEM = {6, 5, 4, 5, 4, 9, 5, 5};
const uint8_t consumableChars[] PROGMEM = {5, 5, 6, 6, 4, 6, 7, 7};

uint8_t itemPartWidth(item::ItemKind kind, uint8_t id, uint8_t part) {
    if (kind == item::ItemKind::Lure) {
        if (id >= item::LURE_COUNT) return 0;
        const uint8_t chars =
            part == 0 ? pgm_read_byte(tierChars + item::lureTierOf(id))
                      : pgm_read_byte(typeChars + item::lureTypeOf(id));
        return static_cast<uint8_t>((chars + 1) * 5);
    }
    if (kind == item::ItemKind::Consumable && id < item::CONSUMABLE_COUNT) {
        return static_cast<uint8_t>((pgm_read_byte(consumableChars + id) + 1) * 5);
    }
    return 0;
}

// Returns the number of name-table lookups this window resolved (lures keep
// two parts, consumables one) so the caller can declare the transition.
uint8_t refreshRows(InventoryMenu &m, const item::Inventory &inv) {
    uint8_t reads = 0;
    for (uint8_t row = 0; row < kVisibleRows; ++row) {
        m.rowId[row] = 0;
        m.rowKind[row] = static_cast<uint8_t>(item::ItemKind::Key);
        m.rowName[row][0] = 0;
        m.rowName[row][1] = 0;
        m.rowParts[row] = 0;
        if (!listViewVisible(m.view, row)) continue;
        item::ItemKind kind;
        uint8_t id;
        const uint8_t nth = listViewItemAt(m.view, row);
        if (!item::inventoryNthNonZero(inv, nth, kind, id)) continue;
        const item::ItemName name = item::itemNameAddr(kind, id);
        reads = static_cast<uint8_t>(reads + name.parts);
        m.rowId[row] = id;
        m.rowKind[row] = static_cast<uint8_t>(kind);
        m.rowName[row][0] = name.part[0];
        m.rowName[row][1] = name.part[1];
        m.rowParts[row] = name.parts;
    }
    return reads;
}

// The world overlay lives here, outside the 193-byte ModeState union.
InventoryMenu bagMenu;
bool bagOpen = false;

uint8_t buildHealMask(const item::Inventory &inv, uint8_t &reads,
                      uint8_t &count) {
    uint8_t mask = 0;
    reads = 0;
    count = 0;
    for (uint8_t id = 0; id < item::CONSUMABLE_COUNT; ++id) {
        if (inv.consumables[id] == 0) continue;
        ++reads;
        if (item::readConsumableDef(id).kind ==
            static_cast<uint8_t>(item::ConsumableKind::Heal)) {
            mask |= static_cast<uint8_t>(1u << id);
            ++count;
        }
    }
    return mask;
}

// The nth set bit is the visible row's semantic consumable id.
uint8_t healNthId(uint8_t mask, uint8_t nth) {
    for (uint8_t id = 0; id < item::CONSUMABLE_COUNT; ++id) {
        if ((mask & (1u << id)) == 0) continue;
        if (nth == 0) return id;
        --nth;
    }
    return 0xFF;
}

// Resolve the visible Heal rows into the shared cache; returns the name reads.
uint8_t refreshHealRows(InventoryMenu &m, const item::Inventory &inv) {
    uint8_t reads = 0;
    for (uint8_t row = 0; row < kVisibleRows; ++row) {
        m.rowId[row] = 0;
        m.rowKind[row] = static_cast<uint8_t>(item::ItemKind::Consumable);
        m.rowName[row][0] = 0;
        m.rowName[row][1] = 0;
        m.rowParts[row] = 0;
        if (!listViewVisible(m.view, row)) continue;
        const uint8_t id = healNthId(m.healMask, listViewItemAt(m.view, row));
        if (id == 0xFF) continue;
        const item::ItemName name =
            item::itemNameAddr(item::ItemKind::Consumable, id);
        ++reads;
        m.rowId[row] = id;
        m.rowName[row][0] = name.part[0];
        m.rowName[row][1] = name.part[1];
        m.rowParts[row] = name.parts;
    }
    return reads;
}
} // namespace

void inventoryMenuOpen(InventoryMenu &m, const item::Inventory &inv) {
    m.view.itemCount = item::inventoryNonZeroCount(inv);
    m.view.rows = kVisibleRows;
    m.view.windowStart = 0;
    m.view.cursor = 0;
    m.healMask = 0;
    FxReadCounter::transitionExact(refreshRows(m, inv));
}

void inventoryMenuOpenHeal(InventoryMenu &m, const item::Inventory &inv) {
    uint8_t reads = 0;
    uint8_t count = 0;
    m.healMask = buildHealMask(inv, reads, count);
    m.view.itemCount = count;
    m.view.rows = kVisibleRows;
    m.view.windowStart = 0;
    m.view.cursor = 0;
    reads = static_cast<uint8_t>(reads + refreshHealRows(m, inv));
    FxReadCounter::transitionExact(reads);
}

void inventoryMenuMoveHeal(InventoryMenu &m, const item::Inventory &inv, int8_t delta) {
    const uint8_t before = m.view.windowStart;
    listViewMove(m.view, delta);
    // Only a scrolled window changes which names are visible.
    const uint8_t reads =
        before != m.view.windowStart ? refreshHealRows(m, inv) : 0;
    FxReadCounter::transitionExact(reads);
}

MenuIntent inventoryMenuSelectHeal(const InventoryMenu &m) {
    if (m.view.itemCount == 0) return {MenuIntentKind::None, 0};
    const uint8_t row = static_cast<uint8_t>(m.view.cursor - m.view.windowStart);
    if (!listViewVisible(m.view, row) || m.rowParts[row] == 0)
        return {MenuIntentKind::None, 0};
    return {MenuIntentKind::SelectItem, m.rowId[row]};
}

void inventoryMenuMove(InventoryMenu &m, const item::Inventory &inv, int8_t delta) {
    const uint8_t before = m.view.windowStart;
    listViewMove(m.view, delta);
    // Cursor moves inside the window keep every cached name; only a scroll
    // changes which items the four rows show.
    if (before != m.view.windowStart)
        FxReadCounter::transitionExact(refreshRows(m, inv));
}

MenuIntent inventoryMenuSelect(const InventoryMenu &m) {
    if (m.view.itemCount == 0) return {MenuIntentKind::None, 0};
    return {MenuIntentKind::SelectItem,
            static_cast<uint8_t>(m.view.cursor - m.view.windowStart)};
}

void drawInventoryMenu(const InventoryMenu &m, const item::Inventory &inv) {
    Blit::fillRect(0, 0, 128, 64, 0);
    for (uint8_t row = 0; row < kVisibleRows; ++row) {
        if (!listViewVisible(m.view, row) || m.rowParts[row] == 0) continue;
        const item::ItemKind kind = static_cast<item::ItemKind>(m.rowKind[row]);
        PpGlyph::number(1, kRowY[row] + 1, item::inventoryCount(inv, kind, m.rowId[row]));
        // Black page with white text; the selected row is a white name cell
        // carrying the inverted (dark) glyph frame, like the battle action
        // rows. The count stays in the black left gutter beside the cell.
        const bool selected = listViewItemAt(m.view, row) == m.view.cursor;
        if (selected) Blit::fillRect(12, kRowY[row], 116, 8, WHITE);
        const uint8_t frame = selected ? 1 : 0;
        drawText(17, kRowY[row], m.rowName[row][0],
                 itemPartWidth(kind, m.rowId[row], 0), frame);
        if (m.rowParts[row] > 1)
            drawText(66, kRowY[row], m.rowName[row][1],
                     itemPartWidth(kind, m.rowId[row], 1), frame);
    }
}

bool inventoryMenuWorldInput(uint8_t edges, bool worldIdle, const item::Inventory &inv) {
    if (!bagOpen) {
        if ((edges & INVENTORY_MENU_B) == 0 || !worldIdle) return false;
        inventoryMenuOpen(bagMenu, inv);
        bagOpen = true;
        return true;
    }
    if (edges & INVENTORY_MENU_B) {
        bagOpen = false;
        return true;
    }
    if (edges & INVENTORY_MENU_UP) {
        inventoryMenuMove(bagMenu, inv, -1);
    } else if (edges & INVENTORY_MENU_DOWN) {
        inventoryMenuMove(bagMenu, inv, 1);
    }
    // A only marks a selection through inventoryMenuSelect(); the overlay
    // never takes an item.
    return true;
}

bool inventoryMenuWorldOpen() { return bagOpen; }

void inventoryMenuWorldDraw(const item::Inventory &inv) {
    if (bagOpen) drawInventoryMenu(bagMenu, inv);
}
