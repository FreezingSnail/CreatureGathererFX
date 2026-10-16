#include "LureZone.hpp"

#include "Chunk.hpp"
#include "../../GameState.hpp"
#include "../../player/Player.hpp"
#include "../../item/ItemNames.hpp"
#include "../../lib/FxRead.hpp"
#include "../../fxdata.h"

extern GameState gameState;
extern Player player;

namespace {
uint8_t gLureId = LureZone::LURE_NONE;
uint8_t gLureCharge = 0;
uint16_t gLureZoneChunk = LureZone::NO_CHUNK;

// Material picker and message state. The cursor/window mirror the shared
// ListView contract without pulling its module into the world-test binary.
bool gPickerOpen = false;
uint8_t gPickerCount = 0;
uint8_t gPickerCursor = 0;
uint8_t gPickerWindow = 0;
bool gPickerFeedback = false;
uint8_t gPickerRowId[LureZone::PICKER_ROWS];
uint8_t gPickerRowParts[LureZone::PICKER_ROWS];
uint24_t gPickerRowName[LureZone::PICKER_ROWS][2];

uint24_t gMessageAddr = 0;
uint8_t gMessageWidth = 0;
uint8_t gMessageTimer = 0;

bool gIntroActive = false;

// Lures precede consumables in inventoryNthNonZero, so the lure-only list is
// that walk with non-lure kinds skipped.
uint8_t lureCount(const item::Inventory &inv) {
    uint8_t total = 0;
    for (uint8_t n = 0;; ++n) {
        item::ItemKind kind;
        uint8_t id;
        if (!item::inventoryNthNonZero(inv, n, kind, id)) break;
        if (kind == item::ItemKind::Lure) ++total;
    }
    return total;
}

bool lureNth(const item::Inventory &inv, uint8_t n, uint8_t &idOut) {
    for (uint8_t k = 0;; ++k) {
        item::ItemKind kind;
        uint8_t id;
        if (!item::inventoryNthNonZero(inv, k, kind, id)) return false;
        if (kind != item::ItemKind::Lure) continue;
        if (n-- == 0) {
            idOut = id;
            return true;
        }
    }
}

void refreshPickerRows() {
    for (uint8_t row = 0; row < LureZone::PICKER_ROWS; ++row) {
        gPickerRowId[row] = 0;
        gPickerRowParts[row] = 0;
        gPickerRowName[row][0] = 0;
        gPickerRowName[row][1] = 0;
        const uint8_t nth = static_cast<uint8_t>(gPickerWindow + row);
        if (nth >= gPickerCount) continue;
        uint8_t id;
        if (!lureNth(player.inventory, nth, id)) continue;
        const item::ItemName name = item::itemNameAddr(item::ItemKind::Lure, id);
        gPickerRowId[row] = id;
        gPickerRowParts[row] = name.parts;
        gPickerRowName[row][0] = name.part[0];
        gPickerRowName[row][1] = name.part[1];
    }
}

// Keeps the cursor inside the list and the window following the cursor.
void clampPicker() {
    if (gPickerCount == 0) {
        gPickerCursor = 0;
        gPickerWindow = 0;
        return;
    }
    if (gPickerCursor >= gPickerCount) gPickerCursor = static_cast<uint8_t>(gPickerCount - 1);
    if (gPickerCursor < gPickerWindow) gPickerWindow = gPickerCursor;
    else if (gPickerCursor >= static_cast<uint8_t>(gPickerWindow + LureZone::PICKER_ROWS))
        gPickerWindow = static_cast<uint8_t>(gPickerCursor - LureZone::PICKER_ROWS + 1);
}
} // namespace

namespace LureZone {

void clear() {
    gLureId = LURE_NONE;
    gLureCharge = 0;
    gLureZoneChunk = NO_CHUNK;
}

void deposit(uint8_t id) {
    if (id == LURE_NONE) {
        clear();
        return;
    }
    gLureId = id;
    gLureCharge = lureChargeOf(lureTier(id));
    gLureZoneChunk = Chunk::chunkOfLocation(gameState.playerLocation);
}

void onStep() {
    if (gMessageTimer) --gMessageTimer;
    if (gLureId == LURE_NONE) return;
    if (gLureCharge > 0) --gLureCharge;
    // Expiry clears the id so the zone reverts to the base table.
    if (gLureCharge == 0) {
        gLureId = LURE_NONE;
        showMessage(lureTextExpired, 50, 40);
    }
}

bool active() { return gLureId != LURE_NONE; }
uint8_t id() { return gLureId; }
uint8_t charge() { return gLureCharge; }
uint16_t zoneChunk() { return gLureZoneChunk; }

uint8_t tableOffset() {
    return active() ? lureType(gLureId) : INERT_OFFSET;
}

item::Inventory &inventory() { return player.inventory; }

bool pickerOpen() { return gPickerOpen; }

void openPicker() {
    gPickerOpen = true;
    gPickerFeedback = false;
    gPickerCursor = 0;
    gPickerWindow = 0;
    gPickerCount = lureCount(player.inventory);
    refreshPickerRows();
}

void closePicker() {
    gPickerOpen = false;
    gPickerFeedback = false;
}

bool pickerDeposit() {
    if (!gPickerOpen || gPickerCount == 0 || gPickerCursor >= gPickerCount) return false;
    uint8_t itemId;
    if (!lureNth(player.inventory, gPickerCursor, itemId)) return false;
    if (!item::inventoryTake(player.inventory, item::ItemKind::Lure, itemId, 1)) return false;
    // Convert the inventory lure id (type * 3 + tier) into the zone encoding.
    deposit(lureId(item::lureTypeOf(itemId), item::lureTierOf(itemId)));
    gPickerFeedback = true;
    gPickerCount = lureCount(player.inventory);
    clampPicker();
    refreshPickerRows();
    return true;
}

void pickerInput(uint8_t buttons) {
    if (!gPickerOpen) return;
    if (buttons & PICKER_BTN_B) {
        closePicker();
        return;
    }
    if (buttons & PICKER_BTN_A) {
        pickerDeposit();
        return;
    }
    if (buttons & PICKER_BTN_UP) {
        if (gPickerCursor > 0) --gPickerCursor;
        gPickerFeedback = false;
        clampPicker();
        refreshPickerRows();
    } else if (buttons & PICKER_BTN_DOWN) {
        if (static_cast<uint8_t>(gPickerCursor + 1) < gPickerCount) ++gPickerCursor;
        gPickerFeedback = false;
        clampPicker();
        refreshPickerRows();
    }
}

bool pickerFeedback() { return gPickerFeedback; }
uint8_t pickerCount() { return gPickerCount; }
uint8_t pickerCursor() { return gPickerCursor; }
uint8_t pickerWindow() { return gPickerWindow; }

bool pickerRow(uint8_t row, uint8_t &id, uint8_t &parts, uint24_t &name0,
               uint24_t &name1) {
    if (row >= PICKER_ROWS || gPickerRowParts[row] == 0) return false;
    id = gPickerRowId[row];
    parts = gPickerRowParts[row];
    name0 = gPickerRowName[row][0];
    name1 = gPickerRowName[row][1];
    return true;
}

void showMessage(uint24_t text, uint8_t width, uint8_t steps) {
    gMessageAddr = text;
    gMessageWidth = width;
    gMessageTimer = steps;
}

bool messageActive() { return gMessageTimer != 0 && gMessageAddr != 0; }
uint24_t messageAddr() { return gMessageAddr; }
uint8_t messageWidth() { return gMessageWidth; }

void showIntro() { gIntroActive = true; }
void dismissIntro() { gIntroActive = false; }
bool introActive() { return gIntroActive; }

void introInput(uint8_t buttons) {
    if (buttons & (PICKER_BTN_UP | PICKER_BTN_DOWN | PICKER_BTN_A | PICKER_BTN_B))
        gIntroActive = false;
}

} // namespace LureZone
