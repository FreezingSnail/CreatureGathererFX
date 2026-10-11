#pragma once

#include <stdint.h>

#include "../../item/Inventory.hpp"
#include "../../item/ItemIds.hpp"
#include "../../lib/uint24.h"

// Overworld lure state for the M2/M3 lure-zone slice. Plain file statics keep
// the deposit charge, selected lure id, owning chunk, the material picker, and
// the world inventory out of WorldTransient, so the mode overlay does not grow.
// StepEvent drives decay and Encounter reads tableOffset() for lure-biased
// table selection.
namespace LureZone {

constexpr uint8_t LURE_NONE = 0xFF;
// Encounter::select() sentinel: no active lure, use the zone's base table.
constexpr uint8_t INERT_OFFSET = 0xFF;
constexpr uint16_t NO_CHUNK = 0xFFFF;

// A lure id packs the type (low two bits, the table selector) and the tier
// (charge class) so a single byte travels through the inventory item id.
constexpr uint8_t lureId(uint8_t type, uint8_t tier) {
    return static_cast<uint8_t>((type & 0x03u) | (tier << 2));
}
constexpr uint8_t lureType(uint8_t id) { return id & 0x03u; }
constexpr uint8_t lureTier(uint8_t id) { return static_cast<uint8_t>(id >> 2); }

// Deposit charge (in steps) per tier. Higher tiers bias encounters longer.
constexpr uint8_t lureChargeOf(uint8_t tier) {
    return static_cast<uint8_t>(4 + (tier < 3 ? tier : 2) * 4);
}

// Reset the zone to inert.
void clear();

// Deposit a lure: sets the id, its tier charge, and the current player chunk
// that owns the zone. A LURE_NONE id clears instead.
void deposit(uint8_t id);

// One overworld step: decay the charge by one, clearing the lure on expiry.
void onStep();

bool active();
uint8_t id();
uint8_t charge();
// The owning chunk recorded at deposit; NO_CHUNK while inert.
uint16_t zoneChunk();
// Lure type (0..3) while active, INERT_OFFSET otherwise.
uint8_t tableOffset();

// Temporary world inventory owner for the M2/M3 gather/deposit slice. Player
// item ownership replaces this in CreatureGathererFX-jp8.5.13.
item::Inventory &inventory();

// --- Material picker ------------------------------------------------------
// The picker lives over the world: up to five visible rows filtered to
// ItemKind::Lure, resolved itemNameAddr caches filled on open and scroll only.
constexpr uint8_t PICKER_ROWS = 5;

constexpr uint8_t PICKER_BTN_UP = 1u << 0;
constexpr uint8_t PICKER_BTN_DOWN = 1u << 1;
constexpr uint8_t PICKER_BTN_A = 1u << 2;
constexpr uint8_t PICKER_BTN_B = 1u << 3;

bool pickerOpen();
void openPicker();
void closePicker();
// Freeze-world input while open: edges move the cursor, A deposits one unit,
// B closes.
void pickerInput(uint8_t buttons);
bool pickerDeposit();
bool pickerFeedback();
uint8_t pickerCount();
uint8_t pickerCursor();
uint8_t pickerWindow();
// Cached visible row; returns false when the row has no lure entry.
bool pickerRow(uint8_t row, uint8_t &id, uint8_t &parts, uint24_t &name0,
               uint24_t &name1);

// --- Short world message --------------------------------------------------
// Non-blocking confirmation text drawn from the world render path.
void showMessage(uint24_t text, uint8_t width, uint8_t steps);
bool messageActive();
uint24_t messageAddr();
uint8_t messageWidth();

// Draws the picker and any active message; defined in LureZoneView.cpp so the
// host/world suites do not pull the Arduboy renderer.
void drawOverlay();

} // namespace LureZone
