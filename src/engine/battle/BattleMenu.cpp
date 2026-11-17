#include "BattleMenu.hpp"
#include "BattleNameCache.hpp"
#include "DuelView.hpp"

#include "../../item/Inventory.hpp"
#include "../../lib/Blit.hpp"
#include "../../lib/FxReadCounter.hpp"
#include "../../lib/Text.hpp"
#include "../menu/InventoryMenu.hpp"
#include <Arduboy2.h>
#ifdef __AVR__
// Keep this identical to the AVR placement-new shim in ModeState.hpp.
inline void *operator new(__SIZE_TYPE__, void *storage) noexcept { return storage; }
inline void operator delete(void *, void *) noexcept {}
#else
#include <new>
#endif
#include "../../fxdata.h"

namespace battle {
namespace {

constexpr uint8_t PAGE_MASK = 0x03;
constexpr uint8_t PAGE_OPTIONS = 0;
constexpr uint8_t PAGE_MOVES = 1;
constexpr uint8_t PAGE_PARTY = 2;
// Duel page aliases. PAGE_MOVES is unused by the duel, so the action page
// takes that slot; PAGE_PARTY is shared with the bench picker and PAGE_OPTIONS
// with the pick row.
constexpr uint8_t PAGE_DUEL_PICK = PAGE_OPTIONS;
constexpr uint8_t PAGE_DUEL_ACTION = PAGE_MOVES;
constexpr uint8_t PAGE_DUEL_ITEMS = 3;
constexpr uint8_t ALLOW_GATHER = 1u << 2;
constexpr uint8_t ALLOW_ESCAPE = 1u << 3;
constexpr uint8_t PASS_SENT = 1u << 4;
constexpr uint8_t PARTY_READY = 1u << 5;
constexpr uint8_t INITIALIZED = 1u << 7;

constexpr uint8_t EDGE_B = 1u << 2;
constexpr uint8_t EDGE_A = 1u << 3;
constexpr uint8_t NAV_DOWN = 1u << 4;
constexpr uint8_t NAV_LEFT = 1u << 5;
constexpr uint8_t NAV_RIGHT = 1u << 6;
constexpr uint8_t NAV_UP = 1u << 7;

constexpr uint8_t MENU_PARTY_SLOT_COUNT = 3;
constexpr uint8_t PARTY_ROWS = 2;
#ifdef CGFX_TRAINER_DEMO_EXPANSION
constexpr uint8_t CREATURE_ID_COUNT = 64;
#else
constexpr uint8_t CREATURE_ID_COUNT = 32;
#endif

MenuIntent noIntent() { return {MenuIntentKind::None, 0}; }
MenuIntent intent(MenuIntentKind kind, uint8_t index = 0) { return {kind, index}; }

// B action rows in a fixed order: GATHER (gatherable wild only), ITEMS (every
// duel), then ESCAPE (wild only). The cursor indexes this list.
uint8_t actionRowCount(uint8_t flags) {
    return static_cast<uint8_t>(1 + ((flags & ALLOW_GATHER) ? 1 : 0) +
                                ((flags & ALLOW_ESCAPE) ? 1 : 0));
}

uint8_t actionItemRow(uint8_t flags) {
    return (flags & ALLOW_GATHER) ? 1 : 0;
}

// The battle bag page reuses the world four-row cache but lives in a file
// static: page storage is mutually exclusive with ModeState and never aliases
// the save buffer. Names are cached on open/scroll only.
InventoryMenu itemPicker;

// Caption widths for the re-homed action labels (5x6 glyphs with a retained
// leading blank cell). ITEMS is its own authored plural: the bag row opens a
// multi-item list, so the singular creature-name sentinel no longer labels it.
constexpr uint8_t gatherWidth = (6 + 1) * 5;
constexpr uint8_t escapeWidth = (6 + 1) * 5;
constexpr uint8_t itemWidth = (5 + 1) * 5;
constexpr uint24_t itemLabel = items;

// One action row: selected cells invert, matching the pick-row convention.
void drawActionRow(uint8_t y, uint8_t width, uint24_t address,
                   uint8_t labelWidth, bool selected) {
    if (selected) Blit::fillRect(0, y, width, 8, WHITE);
    drawText(4, y, address, labelWidth, selected ? 1 : 0);
}

} // namespace

BattleMenu::BattleMenu() : payload_{}, cursor_(0), flags_(0) {}

bool BattleMenu::initialized() const {
    return (flags_ & INITIALIZED) != 0;
}

void BattleMenu::beginDuel(const BattleState &state, bool allowGather,
                           bool allowEscape) {
    payload_.options = duel::usableOptionCount(state, 0);
    for (uint8_t option = 0; option < payload_.options; ++option)
        payload_.slots[option] = option;
    payload_.swap = duel::livingBench(state, 0) ? 1 : 0;
    payload_.wait = payload_.options == 0 ? 1 : 0;
    cursor_ = 0;
    flags_ = static_cast<uint8_t>(INITIALIZED | PAGE_DUEL_PICK |
                                  (allowGather ? ALLOW_GATHER : 0) |
                                  (allowEscape ? ALLOW_ESCAPE : 0));
}

int8_t BattleMenu::duelCursor() const {
    if (!(flags_ & INITIALIZED)) return -1;
    if ((flags_ & PAGE_MASK) != PAGE_DUEL_PICK) return -1;
    return static_cast<int8_t>(cursor_);
}

void BattleMenu::beginParty(const battle::BattleView &view) {
    ::new (static_cast<void *>(&payload_.party)) PartyPayload{};
    const uint8_t player = static_cast<uint8_t>(battle::Side::Player);
    const uint8_t activeSlot = view.activeSlot[player];
    const uint8_t count = view.partyCount[player] > MENU_PARTY_SLOT_COUNT
                              ? MENU_PARTY_SLOT_COUNT
                              : view.partyCount[player];
    uint8_t reads = 0;
    for (uint8_t slot = 0; slot < count; ++slot) {
        if (slot == activeSlot || payload_.party.count >= PARTY_ROWS) continue;
        const battle::PartySummary &summary = view.party[player][slot];
        if (!summary.alive || summary.hp == 0) continue;
        const uint8_t row = payload_.party.count++;
        payload_.party.slots[row] = slot;
        if (summary.id >= CREATURE_ID_COUNT) continue;
        cacheCreatureName(payload_.party.names[row],
                          payload_.party.widths[row], summary.id);
        ++reads;
    }
    FxReadCounter::transitionExact(reads);
    cursor_ = 0;
    flags_ |= PARTY_READY;
}

bool BattleMenu::partyPending() const {
    return (flags_ & PAGE_MASK) == PAGE_PARTY && !(flags_ & PARTY_READY);
}

MenuIntent BattleMenu::update(uint8_t buttons) {
    const uint8_t page = flags_ & PAGE_MASK;

    if (page == PAGE_DUEL_PICK) {
        const uint8_t count = static_cast<uint8_t>(
            payload_.options + payload_.swap + payload_.wait);
        const uint8_t bounded = count == 0 ? 1 : count;
        if (buttons & (NAV_LEFT | NAV_UP))
            cursor_ = cursor_ == 0 ? static_cast<uint8_t>(bounded - 1)
                                   : static_cast<uint8_t>(cursor_ - 1);
        else if (buttons & (NAV_RIGHT | NAV_DOWN))
            cursor_ = static_cast<uint8_t>((cursor_ + 1) % bounded);

        if (buttons & EDGE_B) {
            // Every duel offers ITEMS on the B page, so B always opens it.
            flags_ = static_cast<uint8_t>((flags_ & ~PAGE_MASK) |
                                          PAGE_DUEL_ACTION);
            cursor_ = 0;
            return noIntent();
        }
        if (!(buttons & EDGE_A)) return noIntent();

        if (cursor_ < payload_.options)
            return intent(MenuIntentKind::SelectMove, payload_.slots[cursor_]);
        if (payload_.swap && cursor_ == payload_.options) {
            // SWAP opens the two-row living-bench picker; its names are cached
            // on the page transition by beginParty.
            flags_ = static_cast<uint8_t>((flags_ & ~(PAGE_MASK | PARTY_READY)) |
                                          PAGE_PARTY);
            cursor_ = 0;
            return noIntent();
        }
        if (payload_.wait) {
            if (flags_ & PASS_SENT) return noIntent();
            flags_ |= PASS_SENT;
            return intent(MenuIntentKind::Pass);
        }
        return noIntent();
    }

    if (page == PAGE_DUEL_ACTION) {
        const uint8_t rows = actionRowCount(flags_);
        if (rows > 1) {
            if (buttons & (NAV_UP | NAV_LEFT))
                cursor_ = cursor_ == 0 ? static_cast<uint8_t>(rows - 1)
                                       : static_cast<uint8_t>(cursor_ - 1);
            else if (buttons & (NAV_DOWN | NAV_RIGHT))
                cursor_ = static_cast<uint8_t>((cursor_ + 1) % rows);
        }
        if (buttons & EDGE_B) {
            flags_ = static_cast<uint8_t>((flags_ & ~PAGE_MASK) | PAGE_DUEL_PICK);
            cursor_ = 0;
            return noIntent();
        }
        if (!(buttons & EDGE_A)) return noIntent();
        if ((flags_ & ALLOW_GATHER) && cursor_ == 0)
            return intent(MenuIntentKind::Gather);
        if (cursor_ == actionItemRow(flags_)) {
            // Opening the bag spends nothing; a refusal keeps this choice.
            inventoryMenuOpenHeal(itemPicker, *item::battleInventory());
            flags_ = static_cast<uint8_t>((flags_ & ~PAGE_MASK) |
                                          PAGE_DUEL_ITEMS);
            cursor_ = 0;
            return noIntent();
        }
        if (flags_ & ALLOW_ESCAPE) return intent(MenuIntentKind::Escape);
        return noIntent();
    }

    if (page == PAGE_DUEL_ITEMS) {
        if (buttons & EDGE_B) {
            // Cancel back to the action page with the ITEMS row selected.
            flags_ = static_cast<uint8_t>((flags_ & ~PAGE_MASK) |
                                          PAGE_DUEL_ACTION);
            cursor_ = actionItemRow(flags_);
            return noIntent();
        }
        const item::Inventory &bag = *item::battleInventory();
        if (buttons & (NAV_UP | NAV_LEFT))
            inventoryMenuMoveHeal(itemPicker, bag, -1);
        else if (buttons & (NAV_DOWN | NAV_RIGHT))
            inventoryMenuMoveHeal(itemPicker, bag, 1);
        if (!(buttons & EDGE_A)) return noIntent();
        const MenuIntent selected = inventoryMenuSelectHeal(itemPicker);
        if (selected.kind != MenuIntentKind::None) {
            // An accepted item leaves the page before the concealed interval;
            // a refused use keeps the pick row visible like an attack refusal.
            flags_ = static_cast<uint8_t>((flags_ & ~PAGE_MASK) | PAGE_DUEL_PICK);
            cursor_ = 0;
        }
        return selected;
    }

    if (page == PAGE_PARTY) {
        const uint8_t count = payload_.party.count;
        if (count > 1 && (buttons & (NAV_UP | NAV_DOWN))) cursor_ ^= 1;
        if (buttons & EDGE_B) {
            // Cancel the picker: return to the pick row, on the SWAP cell.
            flags_ = static_cast<uint8_t>((flags_ & ~(PAGE_MASK | PARTY_READY)) |
                                          PAGE_DUEL_PICK);
            cursor_ = payload_.options;
            return noIntent();
        }
        if (!(buttons & EDGE_A)) return noIntent();
        if (cursor_ < count && payload_.party.widths[cursor_] != 0) {
            const uint8_t originalSlot = payload_.party.slots[cursor_];
            return intent(MenuIntentKind::SelectParty, originalSlot);
        }
        return noIntent();
    }

    return noIntent();
}

void BattleMenu::draw() const {
    constexpr uint16_t selectedFrame = 0;
    constexpr uint16_t normalFrame = 1;
    const uint8_t page = flags_ & PAGE_MASK;

    // The pick row + centre are drawn by the duel scene (it owns the resident
    // state); the menu only paints the pages it owns.
    if (page == PAGE_DUEL_PICK) return;

    if (page == PAGE_DUEL_ITEMS) {
        drawInventoryMenu(itemPicker, *item::battleInventory());
        return;
    }

    if (page == PAGE_DUEL_ACTION) {
        // BLACK strip; the selected row is a WHITE cell with dark text. The
        // three-row gatherable page narrows to the free left column so the
        // player sprite (x80..127, y0..47) and HP bar stay visible.
        const uint8_t rows = actionRowCount(flags_);
        const uint8_t width = rows == 3 ? 80 : 128;
        const uint8_t firstY = rows == 3 ? 40 : 48;
        Blit::fillRect(0, firstY, width, static_cast<uint8_t>(rows * 8), BLACK);
        uint8_t row = 0;
        if (flags_ & ALLOW_GATHER) {
            drawActionRow(static_cast<uint8_t>(firstY + row * 8), width, gather,
                          gatherWidth, cursor_ == row);
            ++row;
        }
        drawActionRow(static_cast<uint8_t>(firstY + row * 8), width, itemLabel,
                      itemWidth, cursor_ == row);
        ++row;
        if (flags_ & ALLOW_ESCAPE)
            drawActionRow(static_cast<uint8_t>(firstY + row * 8), width, escape,
                          escapeWidth, cursor_ == row);
        return;
    }

    // The bench picker strip matches the pick row: a black band whose selected
    // row is a white cell.
    Blit::fillRect(0, 48, 128, 16, BLACK);

    if (page == PAGE_PARTY) {
        for (uint8_t row = 0; row < payload_.party.count; ++row) {
            if (payload_.party.widths[row] == 0) continue;
            const uint8_t y = row == 0 ? 48 : 56;
            if (cursor_ == row) {
                // White selected cell, dark text; unselected rows are white text
                // on the black strip, matching the pick row convention.
                Blit::fillRect(0, y, 128, 8, WHITE);
            }
            drawText(4, y, payload_.party.names[row],
                     payload_.party.widths[row],
                     cursor_ == row ? normalFrame : selectedFrame);
        }
    }
}

} // namespace battle
