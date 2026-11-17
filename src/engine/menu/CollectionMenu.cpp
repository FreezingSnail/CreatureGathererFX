#include "CollectionMenu.hpp"

#include <avr/pgmspace.h>

#include "../../common.hpp"
#include "../../creature/LevelCurve.hpp"
#include "../../lib/Blit.hpp"
#include "../../lib/FxReadCounter.hpp"
#include "../../lib/ReadData.hpp"
#include "../../lib/Text.hpp"
#include "../../player/Player.hpp"
#include "../../save/Compaction.hpp"
#include "../../save/SaveController.hpp"
#include "../PpGlyph.hpp"
#include "InventoryMenu.hpp"

namespace {
constexpr uint8_t kVisibleRows = 4;
constexpr uint8_t kRowY[kVisibleRows] = {12, 24, 36, 48};
// BAG=0, COLLECTION=1, SAVE=2.
constexpr uint8_t kRootItems = 3;
constexpr uint8_t kRootBag = 0;
constexpr uint8_t kRootCollection = 1;
constexpr uint8_t kNameMaxWidth = 80;
constexpr uint8_t kPartySlots = 3;
constexpr uint8_t kNoSlot = 255;
constexpr uint8_t kFeedbackNone = static_cast<uint8_t>(CollectionFeedback::None);
constexpr uint8_t kFeedbackInParty = static_cast<uint8_t>(CollectionFeedback::InParty);
constexpr uint8_t kFeedbackSaveNeeded =
    static_cast<uint8_t>(CollectionFeedback::SaveNeeded);

const char menuLabel[] PROGMEM = "MENU";
const char bagLabel[] PROGMEM = "BAG";
const char collectionLabel[] PROGMEM = "COLLECTION";
const char saveLabel[] PROGMEM = "SAVE";
const char partyLabel[] PROGMEM = "PARTY";
const char alreadyInPartyLabel[] PROGMEM = "ALREADY IN PARTY";
const char saveNeededLabel[] PROGMEM = "SAVE NEEDED";

void drawLabel(uint8_t x, uint8_t y, const char *text) {
    char character;
    while ((character = static_cast<char>(pgm_read_byte(text++))) != '\0') {
        drawGlyph(x, y, static_cast<uint8_t>(character), Blit::OVERWRITE);
        x += 5;
    }
}

bool partyHasSlot(uint8_t slot) {
    for (uint8_t i = 0; i < PARTY_MAX; ++i)
        if (storePartySlot(i) == slot) return true;
    return false;
}

// One bounded occupancy pass over the stable slots. Each visited slot is
// decoded one record at a time; only the four window rows cache a name
// address/width/level/party flag. Save-flash reads are not program-data
// metadata reads, and nothing here is retained for off-window instances.
// Returns the name-table lookups performed so the caller can declare them.
uint8_t refreshRows(CollectionMenu &m) {
    for (uint8_t row = 0; row < kVisibleRows; ++row) {
        m.rowSlot[row] = kNoSlot;
        m.rowId[row] = 0;
        m.rowLevel[row] = 0;
        m.rowWidth[row] = 0;
        m.rowName[row] = 0;
    }
    m.rowParty = 0;
    uint8_t occupied = 0;
    uint8_t reads = 0;
    for (uint8_t slot = 0; slot < STORE_CAPACITY; ++slot) {
        StoreRecord record;
        if (!storeRead(slot, record) || record.id == STORE_RECORD_TOMBSTONE_ID)
            continue;
        if (occupied >= m.view.windowStart &&
            static_cast<uint8_t>(occupied - m.view.windowStart) < kVisibleRows) {
            const uint8_t row = static_cast<uint8_t>(occupied - m.view.windowStart);
            m.rowSlot[row] = slot;
            m.rowId[row] = record.id;
            m.rowLevel[row] = levelFromExp(record.exp);
            const uint8_t width = readCreatureNameWidth(record.id);
            m.rowWidth[row] = width > kNameMaxWidth ? kNameMaxWidth : width;
            m.rowName[row] = readCreatureNameAddress(record.id);
            ++reads;
            if (partyHasSlot(slot)) m.rowParty |= static_cast<uint8_t>(1u << row);
        }
        ++occupied;
    }
    m.view.itemCount = occupied;
    return reads;
}

void drawRoot(const CollectionMenu &m) {
    drawLabel(0, 0, menuLabel);
    for (uint8_t item = 0; item < kRootItems; ++item) {
        // BAG/COLLECTION/SAVE at the frozen .23 root layout: 16px row pitch so
        // the third action sits at y44, marker x2, labels x12.
        const uint8_t y = static_cast<uint8_t>(12 + item * 16);
        if (item == m.rootCursor) drawGlyph(2, y, '>', Blit::OVERWRITE);
        drawLabel(12, y,
                  item == kRootBag ? bagLabel
                                   : item == kRootCollection ? collectionLabel
                                                             : saveLabel);
    }
}

void drawList(const CollectionMenu &m) {
    drawLabel(0, 0, collectionLabel);
    for (uint8_t row = 0; row < kVisibleRows; ++row) {
        if (m.rowSlot[row] == kNoSlot) continue;
        const uint8_t y = kRowY[row];
        const bool selected =
            listViewItemAt(m.view, row) == m.view.cursor;
        if (selected) drawGlyph(0, y, '>', Blit::OVERWRITE);
        // Selected name cell inverts (white fill, dark glyphs); level and party
        // marker stay on the black right gutter outside the 80-px cell.
        if (selected) Blit::fillRect(8, y, kNameMaxWidth, 8, WHITE);
        if (m.rowWidth[row] != 0)
            drawText(8, y, m.rowName[row], m.rowWidth[row],
                     FRAME(selected ? 1 : 0));
        PpGlyph::number(92, static_cast<uint8_t>(y + 1), m.rowLevel[row]);
        if ((m.rowParty & static_cast<uint8_t>(1u << row)) != 0)
            drawGlyph(122, y, 'P', Blit::OVERWRITE);
    }
    // Feedback sits below the four instance rows so the cache stays unchanged.
    if (m.feedback == kFeedbackInParty)
        drawLabel(0, 56, alreadyInPartyLabel);
    else if (m.feedback == kFeedbackSaveNeeded)
        drawLabel(0, 56, saveNeededLabel);
}

void drawParty(const CollectionMenu &m) {
    drawLabel(0, 0, partyLabel);
    for (uint8_t slot = 0; slot < kPartySlots; ++slot) {
        const uint8_t y = kRowY[slot];
        if (slot == m.slotCursor) drawGlyph(0, y, '>', Blit::OVERWRITE);
        drawGlyph(8, y, static_cast<uint8_t>('1' + slot), Blit::OVERWRITE);
    }
}

// The world menu lives here, outside the 193-byte ModeState union.
CollectionMenu worldMenu;

void openRoot() {
    worldMenu.page = static_cast<uint8_t>(CollectionPage::Root);
    worldMenu.rootCursor = 0;
    worldMenu.slotCursor = 0;
    worldMenu.pendingSlot = kNoSlot;
    worldMenu.choiceSlot = kNoSlot;
    worldMenu.choiceParty = 0;
    worldMenu.feedback = kFeedbackNone;
}
} // namespace

void collectionMenuOpen(CollectionMenu &m) {
    m.view.rows = kVisibleRows;
    m.view.windowStart = 0;
    m.view.cursor = 0;
    m.view.itemCount = 0;
    FxReadCounter::transitionExact(refreshRows(m));
}

void collectionMenuMove(CollectionMenu &m, int8_t delta) {
    const uint8_t before = m.view.windowStart;
    listViewMove(m.view, delta);
    // Cursor moves inside the window keep the cached rows; only a scroll
    // changes which instances the four rows show.
    if (before != m.view.windowStart)
        FxReadCounter::transitionExact(refreshRows(m));
}

uint8_t collectionMenuSelect(const CollectionMenu &m) {
    if (m.view.itemCount == 0) return kNoSlot;
    return m.rowSlot[m.view.cursor - m.view.windowStart];
}

void drawCollectionMenu(const CollectionMenu &m) {
    Blit::fillRect(0, 0, 128, 64, 0);
    switch (static_cast<CollectionPage>(m.page)) {
    case CollectionPage::Root: drawRoot(m); break;
    case CollectionPage::List: drawList(m); break;
    case CollectionPage::Party: drawParty(m); break;
    default: break;
    }
}

bool collectionMenuWorldOpen() {
    return static_cast<CollectionPage>(worldMenu.page) != CollectionPage::Closed;
}

CollectionPage collectionMenuWorldPage() {
    return static_cast<CollectionPage>(worldMenu.page);
}

CollectionChoice collectionMenuWorldChoice() {
    return {worldMenu.choiceSlot, worldMenu.choiceParty};
}

CollectionFeedback collectionMenuWorldFeedback() {
    return static_cast<CollectionFeedback>(worldMenu.feedback);
}

bool collectionMenuWorldApplyChoice(Player &player) {
    if (worldMenu.choiceSlot == kNoSlot) return false;
    const uint8_t slot = worldMenu.choiceSlot;
    const uint8_t index = static_cast<uint8_t>(worldMenu.choiceParty - 1);
    // Consume the choice before publication so a refusal cannot retry behind
    // the player's back; a new selection is required.
    worldMenu.choiceSlot = kNoSlot;
    for (uint8_t i = 0; i < PARTY_MAX; ++i) {
        if (storePartySlot(i) == slot) {
            worldMenu.feedback = kFeedbackInParty;
            return true;
        }
    }
    // First publish the outgoing member's current HP to its own record. Only
    // the HP byte changes, so exp and moves stay exact. A refusal here leaves
    // the live party and HP untouched.
    StoreRecord record;
    const uint8_t outgoing = storePartySlot(index);
    if (outgoing != kNoSlot) {
        if (!storeRead(outgoing, record)) {
            worldMenu.feedback = kFeedbackSaveNeeded;
            return true;
        }
        record.hpEncoded = static_cast<uint8_t>(player.creatureHPs[index] + 1);
        if (!storeWrite(outgoing, record)) {
            worldMenu.feedback = kFeedbackSaveNeeded;
            return true;
        }
    }
    // Legacy zero encodes full HP. The assignment must publish before the live
    // party changes; the earlier outgoing-HP mirror remains a valid mutation.
    if (!storeRead(slot, record) ||
        record.id == STORE_RECORD_TOMBSTONE_ID) {
        worldMenu.feedback = kFeedbackSaveNeeded;
        return true;
    }
    const uint8_t hp = storeInstanceHP(record);
    if (!storeAssign(index, slot, hp)) {
        worldMenu.feedback = kFeedbackSaveNeeded;
        return true;
    }
    // Reconstruct only the selected member, installing its exact current HP.
    // The journaled write/assign validations and this reconstruction are the
    // swap's species lookups; the encoded HP decides whether a max-HP read is
    // also needed, so declare the whole boundary instead of a fixed count.
    player.party[index].load(record);
    FxReadCounter::transitionCurrent();
    player.creatureHPs[index] = hp;
    worldMenu.feedback = kFeedbackNone;
    return true;
}

bool collectionMenuWorldInput(uint8_t edges, bool worldIdle,
                              const item::Inventory &inv) {
    switch (static_cast<CollectionPage>(worldMenu.page)) {
    case CollectionPage::Closed:
        if ((edges & INVENTORY_MENU_B) == 0 || !worldIdle) return false;
        openRoot();
        return true;
    case CollectionPage::Bag:
        // The bag owns its four-row page and edges; B returns to this root.
        inventoryMenuWorldInput(edges, true, inv);
        if (!inventoryMenuWorldOpen())
            worldMenu.page = static_cast<uint8_t>(CollectionPage::Root);
        return true;
    case CollectionPage::Root:
        if (edges & INVENTORY_MENU_B) {
            worldMenu.page = static_cast<uint8_t>(CollectionPage::Closed);
        } else if (edges & INVENTORY_MENU_UP) {
            if (worldMenu.rootCursor > 0) --worldMenu.rootCursor;
        } else if (edges & INVENTORY_MENU_DOWN) {
            if (worldMenu.rootCursor + 1 < kRootItems) ++worldMenu.rootCursor;
        } else if (edges & INVENTORY_MENU_A) {
            if (worldMenu.rootCursor == kRootBag) {
                // Fresh composite open: the bag's own static tracks its page.
                inventoryMenuWorldInput(INVENTORY_MENU_B, true, inv);
                worldMenu.page = static_cast<uint8_t>(CollectionPage::Bag);
            } else if (worldMenu.rootCursor == kRootCollection) {
                collectionMenuOpen(worldMenu);
                worldMenu.page = static_cast<uint8_t>(CollectionPage::List);
            } else {
                // Explicit checkpoint: close the menu, then snapshot live RAM.
                // SAVING owns the frame until it finishes or the failure
                // screen's fresh A/B resolves it, so this run cannot repeat.
                worldMenu.page = static_cast<uint8_t>(CollectionPage::Closed);
                SaveController::begin(GameState_t::WORLD);
            }
        }
        return true;
    case CollectionPage::List:
        if (edges & INVENTORY_MENU_B) {
            worldMenu.page = static_cast<uint8_t>(CollectionPage::Root);
        } else if (edges & INVENTORY_MENU_UP) {
            collectionMenuMove(worldMenu, -1);
        } else if (edges & INVENTORY_MENU_DOWN) {
            collectionMenuMove(worldMenu, 1);
        } else if (edges & INVENTORY_MENU_A) {
            const uint8_t slot = collectionMenuSelect(worldMenu);
            if (slot != kNoSlot) {
                worldMenu.pendingSlot = slot;
                worldMenu.slotCursor = 0;
                worldMenu.feedback = kFeedbackNone;
                worldMenu.page = static_cast<uint8_t>(CollectionPage::Party);
            }
        }
        return true;
    case CollectionPage::Party:
        if (edges & INVENTORY_MENU_B) {
            worldMenu.page = static_cast<uint8_t>(CollectionPage::List);
        } else if (edges & INVENTORY_MENU_UP) {
            if (worldMenu.slotCursor > 0) --worldMenu.slotCursor;
        } else if (edges & INVENTORY_MENU_DOWN) {
            if (worldMenu.slotCursor + 1 < kPartySlots) ++worldMenu.slotCursor;
        } else if (edges & INVENTORY_MENU_A) {
            worldMenu.choiceSlot = worldMenu.pendingSlot;
            worldMenu.choiceParty = static_cast<uint8_t>(worldMenu.slotCursor + 1);
            worldMenu.page = static_cast<uint8_t>(CollectionPage::List);
        }
        return true;
    }
    return true;
}

void collectionMenuWorldDraw(const item::Inventory &inv) {
    if (worldMenu.page == static_cast<uint8_t>(CollectionPage::Bag))
        inventoryMenuWorldDraw(inv);
    else
        drawCollectionMenu(worldMenu);
}
