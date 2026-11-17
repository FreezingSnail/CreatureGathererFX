#pragma once

#include <string.h>

#include "test.hpp"
#include "src/FXDataFake.hpp"
#include "../src/creature/LevelCurve.hpp"
#include "../src/engine/menu/CollectionMenu.hpp"
#include "../src/engine/menu/InventoryMenu.hpp"
#include "../src/engine/world/LureZone.hpp"
#include "../src/globals.hpp"
#include "../src/item/Inventory.hpp"
#include "../src/item/ItemIds.hpp"
#include "../src/lib/FxReadCounter.hpp"
#include "../src/save/Compaction.hpp"
#include "../src/save/FlashBackend.hpp"
#include "../src/save/SaveController.hpp"

namespace collection_menu_test_detail {
void fresh() { flashFakeReset(); }

StoreRecord creature(uint8_t id, uint16_t exp, uint8_t hpEncoded = 0) {
    StoreRecord record = {};
    record.id = id;
    record.exp = exp;
    record.attackMask = 2;
    record.hpEncoded = hpEncoded;
    return record;
}

uint8_t fullHP(const StoreRecord &record) {
    Creature instance;
    instance.load(record);
    return instance.hp;
}

void closeWorldMenu() {
    for (uint8_t i = 0; i < 4 && collectionMenuWorldOpen(); ++i)
        collectionMenuWorldInput(INVENTORY_MENU_B, false, LureZone::inventory());
    if (inventoryMenuWorldOpen())
        inventoryMenuWorldInput(INVENTORY_MENU_B, true, LureZone::inventory());
}

// Drive the real root -> collection -> party-slot path. Fixtures in this test
// allocate contiguous stable slots from zero, so row index equals stable slot.
void chooseSlot(uint8_t slot, uint8_t partySlot) {
    closeWorldMenu();
    collectionMenuWorldInput(INVENTORY_MENU_B, true, LureZone::inventory());
    collectionMenuWorldInput(INVENTORY_MENU_DOWN, false, LureZone::inventory());
    collectionMenuWorldInput(INVENTORY_MENU_A, false, LureZone::inventory());
    for (uint8_t row = 0; row < slot; ++row)
        collectionMenuWorldInput(INVENTORY_MENU_DOWN, false, LureZone::inventory());
    collectionMenuWorldInput(INVENTORY_MENU_A, false, LureZone::inventory());
    for (uint8_t i = 0; i < partySlot; ++i)
        collectionMenuWorldInput(INVENTORY_MENU_DOWN, false, LureZone::inventory());
    collectionMenuWorldInput(INVENTORY_MENU_A, false, LureZone::inventory());
}

SaveFile snapshot() {
    SaveFile state = {};
    memset(state.partySlots, 255, 3);
    return state;
}

SaveStep save(SaveFile &state) {
    saveBegin();
    SaveStep result = SaveStep::Idle;
    for (uint16_t i = 0; i < 256 && saveInProgress(); ++i)
        result = saveStepAdvance(state);
    return result;
}
} // namespace collection_menu_test_detail

// jp8.11.8: four-row stable-slot collection view plus the world BAG/COLLECTION
// root menu. The suite proves occupancy/tombstone/duplicate/slot63/full64
// ordering, transition-only name reads and the root/page/cancel state machine.
void CollectionMenuTest(TestSuite &suite) {
    using namespace collection_menu_test_detail;
    Test test(__func__);
    CollectionMenu menu = {};

    // Empty store: no rows, no cached facts, no metadata lookup.
    flashFakeReset();
    FxReadCounter::resetFrame();
    collectionMenuOpen(menu);
    test.assert(menu.view.itemCount, static_cast<uint8_t>(0), "empty collection has no rows");
    test.assert(menu.rowSlot[0], static_cast<uint8_t>(255), "empty row cache is clear");
    test.assert(FxReadCounter::count(), static_cast<uint8_t>(0),
                "opening an empty collection resolves no names");
    test.assert(collectionMenuSelect(menu), static_cast<uint8_t>(255),
                "empty collection selects nothing");
    drawCollectionMenu(menu);
    test.assert(FxReadCounter::count(), static_cast<uint8_t>(0),
                "drawing the empty page resolves no names");
    collectionMenuMove(menu, 1);
    test.assert(menu.view.cursor, static_cast<uint8_t>(0), "empty page cannot move");
    collectionMenuMove(menu, -1);
    test.assert(menu.view.cursor, static_cast<uint8_t>(0), "empty page never wraps");

    // Duplicate species stay separate rows; a tombstone is skipped; the party
    // marker follows the stable slot, not the row or species.
    flashFakeReset();
    StoreRecord first = creature(3, 27);
    StoreRecord duplicate = creature(3, 125);
    StoreRecord removed = creature(5, 8);
    uint8_t slot0 = 255;
    uint8_t slot1 = 255;
    uint8_t slot2 = 255;
    test.assert(storeAdd(first, slot0), true, "first duplicate accepted");
    test.assert(storeAdd(duplicate, slot1), true, "duplicate species accepted");
    test.assert(storeAdd(removed, slot2), true, "tombstone candidate accepted");
    test.assert(storeRemove(slot2), true, "candidate tombstoned");
    test.assert(storeAssign(0, slot0, 0), true, "party assignment accepted");
    FxReadCounter::resetFrame();
    collectionMenuOpen(menu);
    test.assert(menu.view.itemCount, static_cast<uint8_t>(2), "tombstone is not counted");
    test.assert(menu.rowSlot[0], slot0, "first row keeps the first stable slot");
    test.assert(menu.rowSlot[1], slot1, "duplicate owns its own row and slot");
    test.assert(menu.rowId[0], static_cast<uint8_t>(3), "first row species");
    test.assert(menu.rowId[1], static_cast<uint8_t>(3),
                "duplicate row keeps the same species, not deduped");
    test.assert(menu.rowLevel[0], levelFromExp(27), "first row derives level from exp");
    test.assert(menu.rowLevel[1], levelFromExp(125), "duplicate derives its own level");
    test.assert(menu.rowWidth[0], static_cast<uint8_t>(60), "cached name width");
    test.assert(menu.rowName[0], static_cast<uint24_t>(0x10003),
                "cached name address");
    test.assert(menu.rowParty, static_cast<uint8_t>(0b01),
                "only the assigned stable slot shows the party marker");
    test.assert(FxReadCounter::count(), static_cast<uint8_t>(2),
                "open resolves only the two occupied visible names");
    const uint8_t duplicateReads = FxReadCounter::count();
    drawCollectionMenu(menu);
    test.assert(FxReadCounter::count(), duplicateReads,
                "draw reads cached names only");
    collectionMenuMove(menu, 1);
    test.assert(menu.view.cursor, static_cast<uint8_t>(1), "move advances selection");
    test.assert(FxReadCounter::count(), duplicateReads,
                "in-window move performs no name lookup");
    test.assert(collectionMenuSelect(menu), slot1,
                "selection carries the duplicate's own stable slot");

    // Eight occupied rows: the window follows the cursor, stops at both ends
    // and refreshes exactly the four visible rows on a scroll.
    flashFakeReset();
    for (uint8_t i = 0; i < 8; ++i) {
        StoreRecord row = creature(static_cast<uint8_t>(10 + i),
                                   levelExpThreshold(1) + i);
        uint8_t slot = 255;
        test.assert(storeAdd(row, slot), true, "scroll fixture accepted");
    }
    FxReadCounter::resetFrame();
    collectionMenuOpen(menu);
    test.assert(menu.view.itemCount, static_cast<uint8_t>(8), "eight occupied rows");
    test.assert(menu.rowSlot[3], static_cast<uint8_t>(3), "first window ends at slot 3");
    test.assert(FxReadCounter::count(), static_cast<uint8_t>(4),
                "open resolves four visible names once");
    collectionMenuMove(menu, 100);
    test.assert(menu.view.cursor, static_cast<uint8_t>(7), "down clamps at the last row");
    test.assert(menu.view.windowStart, static_cast<uint8_t>(4), "end window holds four rows");
    test.assert(menu.rowSlot[0], static_cast<uint8_t>(4), "scrolled window starts at slot 4");
    test.assert(menu.rowSlot[3], static_cast<uint8_t>(7), "scrolled window ends at slot 7");
    test.assert(FxReadCounter::count(), static_cast<uint8_t>(8),
                "scroll resolves only the shifted window");
    test.assert(collectionMenuSelect(menu), static_cast<uint8_t>(7),
                "scrolled selection is the stable slot");
    collectionMenuMove(menu, 1);
    test.assert(menu.view.cursor, static_cast<uint8_t>(7), "down at the end does not wrap");
    collectionMenuMove(menu, -100);
    test.assert(menu.view.cursor, static_cast<uint8_t>(0), "up clamps at the first row");
    test.assert(menu.view.windowStart, static_cast<uint8_t>(0), "window returns to the start");
    test.assert(menu.rowSlot[0], static_cast<uint8_t>(0), "scroll back refreshes row 0");
    test.assert(FxReadCounter::count(), static_cast<uint8_t>(12),
                "scroll back refreshes one window");
    collectionMenuMove(menu, -1);
    test.assert(menu.view.cursor, static_cast<uint8_t>(0), "up at the start does not wrap");

    // Last stable slot 63 is reachable and keeps its identity.
    flashFakeReset();
    test.assert(storeWrite(63, creature(9, 1)), true, "slot 63 accepts an instance");
    FxReadCounter::resetFrame();
    collectionMenuOpen(menu);
    test.assert(menu.view.itemCount, static_cast<uint8_t>(1), "slot 63 is listed");
    test.assert(menu.rowSlot[0], static_cast<uint8_t>(63), "slot 63 keeps its stable identity");
    test.assert(collectionMenuSelect(menu), static_cast<uint8_t>(63),
                "slot 63 selection stays 63");
    test.assert(FxReadCounter::count(), static_cast<uint8_t>(1), "one visible name resolved");

    // Full 64-slot collection: ascending stable order, slot 63 selectable.
    flashFakeReset();
    SaveFile state = snapshot();
    uint8_t lastSlot = 255;
    for (uint8_t i = 0; i < 64; ++i) {
        StoreRecord row = creature(static_cast<uint8_t>(i % 32),
                                   levelExpThreshold(1) + i);
        uint8_t slot = 255;
        test.assert(storeAdd(row, slot), true, "bounded full-collection add");
        test.assert(slot, i, "stable slot allocation");
        lastSlot = slot;
        if ((i & 15) == 15)
            test.assert(save(state), SaveStep::Done, "explicit batch checkpoint");
    }
    test.assert(storeAdd(creature(0, 1), lastSlot), false, "full64 refuses an add");
    FxReadCounter::resetFrame();
    collectionMenuOpen(menu);
    test.assert(menu.view.itemCount, static_cast<uint8_t>(64), "full collection lists 64 rows");
    test.assert(menu.rowSlot[0], static_cast<uint8_t>(0), "full window starts at slot 0");
    test.assert(menu.rowSlot[3], static_cast<uint8_t>(3), "full window holds slots 0..3");
    test.assert(FxReadCounter::count(), static_cast<uint8_t>(4),
                "full open resolves only four names");
    collectionMenuMove(menu, 100);
    test.assert(menu.view.cursor, static_cast<uint8_t>(63), "full collection reaches the last row");
    test.assert(menu.view.windowStart, static_cast<uint8_t>(60), "last window starts at row 60");
    test.assert(menu.rowSlot[0], static_cast<uint8_t>(60), "window starts at slot 60");
    test.assert(menu.rowSlot[3], static_cast<uint8_t>(63), "window ends at slot 63");
    test.assert(collectionMenuSelect(menu), static_cast<uint8_t>(63),
                "last selection is the real slot 63");
    test.assert(FxReadCounter::count(), static_cast<uint8_t>(8),
                "full scroll refreshes one window");
    drawCollectionMenu(menu);
    test.assert(FxReadCounter::count(), static_cast<uint8_t>(8),
                "full page draw performs no metadata lookup");

    // World root menu: BAG/COLLECTION, reserved SAVE slot absent, fresh edges
    // only, B closes, and each page freezes the world by owning the frame.
    flashFakeReset();
    item::inventoryClear(LureZone::inventory());
    item::inventoryAdd(LureZone::inventory(), item::ItemKind::Lure,
                       item::lureId(1, 0), 1);
    if (inventoryMenuWorldOpen())
        inventoryMenuWorldInput(INVENTORY_MENU_B, true, LureZone::inventory());
    test.assert(collectionMenuWorldInput(INVENTORY_MENU_B, false, LureZone::inventory()),
                false, "B cannot open the menu while the world is busy");
    test.assert(collectionMenuWorldOpen(), false, "busy world leaves the menu closed");
    test.assert(collectionMenuWorldInput(INVENTORY_MENU_B, true, LureZone::inventory()),
                true, "fresh B on an idle world opens the root menu");
    test.assert(collectionMenuWorldPage(), CollectionPage::Root, "root page is current");
    test.assert(collectionMenuWorldChoice().slot, static_cast<uint8_t>(255),
                "opening clears the previous choice");
    test.assert(collectionMenuWorldInput(INVENTORY_MENU_UP, false, LureZone::inventory()),
                true, "open menu owns the frame even when the world is not idle");
    test.assert(collectionMenuWorldPage(), CollectionPage::Root, "root cursor stays on BAG");
    test.assert(collectionMenuWorldInput(INVENTORY_MENU_A, false, LureZone::inventory()),
                true, "A opens the selected page");
    test.assert(inventoryMenuWorldOpen(), true, "A on BAG opens the four-row bag page");
    test.assert(collectionMenuWorldPage(), CollectionPage::Bag, "bag page is current");
    test.assert(collectionMenuWorldInput(INVENTORY_MENU_B, false, LureZone::inventory()),
                true, "B closes the bag page");
    test.assert(inventoryMenuWorldOpen(), false, "bag page closes");
    test.assert(collectionMenuWorldPage(), CollectionPage::Root,
                "B from the bag returns to the root menu");
    test.assert(collectionMenuWorldInput(INVENTORY_MENU_DOWN, false, LureZone::inventory()),
                true, "down moves the root cursor");
    test.assert(collectionMenuWorldInput(INVENTORY_MENU_DOWN, false, LureZone::inventory()),
                true, "down moves the root cursor to SAVE");
    test.assert(collectionMenuWorldInput(INVENTORY_MENU_DOWN, false, LureZone::inventory()),
                true, "down clamps at SAVE");
    test.assert(collectionMenuWorldInput(INVENTORY_MENU_UP, false, LureZone::inventory()),
                true, "up returns the root cursor to COLLECTION");
    test.assert(collectionMenuWorldInput(INVENTORY_MENU_A, false, LureZone::inventory()),
                true, "A opens COLLECTION");
    test.assert(collectionMenuWorldPage(), CollectionPage::List, "collection list is current");
    test.assert(collectionMenuWorldInput(INVENTORY_MENU_A, false, LureZone::inventory()),
                true, "A on an empty collection keeps the page");
    test.assert(collectionMenuWorldPage(), CollectionPage::List,
                "empty collection cannot open a party choice");
    test.assert(collectionMenuWorldInput(INVENTORY_MENU_B, false, LureZone::inventory()),
                true, "B returns to the root menu");
    test.assert(collectionMenuWorldPage(), CollectionPage::Root, "root page after B");
    test.assert(collectionMenuWorldInput(INVENTORY_MENU_B, false, LureZone::inventory()),
                true, "B closes the root menu");
    test.assert(collectionMenuWorldOpen(), false, "root B leaves the world frame free");

    // jp8.11.23: SAVE is the third root action. A closes the menu and starts
    // exactly one explicit WORLD checkpoint; the now-closed menu cannot re-enter
    // it, and Done returns to WORLD.
    gameState.state = GameState_t::WORLD;
    test.assert(collectionMenuWorldInput(INVENTORY_MENU_B, true, LureZone::inventory()),
                true, "fresh B reopens the root for SAVE");
    collectionMenuWorldInput(INVENTORY_MENU_DOWN, false, LureZone::inventory());
    collectionMenuWorldInput(INVENTORY_MENU_DOWN, false, LureZone::inventory());
    FxReadCounter::resetFrame();
    test.assert(collectionMenuWorldInput(INVENTORY_MENU_A, false, LureZone::inventory()),
                true, "A on SAVE consumes the frame");
    test.assert(gameState.state, GameState_t::SAVING, "A on SAVE enters the save state");
    test.assert(collectionMenuWorldOpen(), false, "SAVE closes the world menu");
    test.assert(saveInProgress(), true, "SAVE starts one snapshot");
    test.assert(FxReadCounter::count(), static_cast<uint8_t>(0),
                "SAVE selection reads no FX metadata");
    for (uint16_t frame = 0; frame < 256 && saveInProgress(); ++frame)
        SaveController::advance();
    test.assert(saveInProgress(), false, "menu checkpoint completes");
    test.assert(gameState.state, GameState_t::WORLD, "menu checkpoint returns to WORLD");
    test.assert(collectionMenuWorldInput(0, true, LureZone::inventory()), false,
                "a held activation leaves the menu closed");

    // Populate two instances and drive the collection page -> party slot page.
    uint8_t worldSlot = 255;
    test.assert(storeAdd(creature(6, levelExpThreshold(3)), worldSlot), true,
                "world selection fixture accepted");
    FxReadCounter::resetFrame();
    test.assert(collectionMenuWorldInput(INVENTORY_MENU_B, true, LureZone::inventory()),
                true, "fresh B reopens the root menu");
    test.assert(FxReadCounter::count(), static_cast<uint8_t>(0),
                "root page performs no metadata lookup");
    collectionMenuWorldInput(INVENTORY_MENU_DOWN, false, LureZone::inventory());
    test.assert(collectionMenuWorldInput(INVENTORY_MENU_A, false, LureZone::inventory()),
                true, "A opens the populated collection");
    test.assert(collectionMenuWorldPage(), CollectionPage::List, "list page is current");
    test.assert(collectionMenuWorldInput(INVENTORY_MENU_A, false, LureZone::inventory()),
                true, "A opens party-slot selection");
    test.assert(collectionMenuWorldPage(), CollectionPage::Party, "party page is current");
    test.assert(collectionMenuWorldChoice().slot, static_cast<uint8_t>(255),
                "no choice is committed before A");
    test.assert(collectionMenuWorldInput(INVENTORY_MENU_B, false, LureZone::inventory()),
                true, "B cancels party-slot selection");
    test.assert(collectionMenuWorldPage(), CollectionPage::List, "cancel returns to the list");
    test.assert(collectionMenuWorldChoice().slot, static_cast<uint8_t>(255),
                "cancel commits no choice");
    collectionMenuWorldInput(INVENTORY_MENU_A, false, LureZone::inventory());
    test.assert(collectionMenuWorldInput(INVENTORY_MENU_DOWN, false, LureZone::inventory()),
                true, "down moves the party choice to slot 2");
    test.assert(collectionMenuWorldInput(INVENTORY_MENU_A, false, LureZone::inventory()),
                true, "A confirms the party choice");
    test.assert(collectionMenuWorldPage(), CollectionPage::List,
                "confirmation returns to the collection list");
    test.assert(collectionMenuWorldChoice().slot, worldSlot,
                "choice carries the instance's stable slot");
    test.assert(collectionMenuWorldChoice().partySlot, static_cast<uint8_t>(2),
                "choice carries the selected party slot");
    test.assert(collectionMenuWorldInput(INVENTORY_MENU_B, false, LureZone::inventory()),
                true, "B leaves the collection page");
    test.assert(collectionMenuWorldPage(), CollectionPage::Root, "root page after leaving list");
    test.assert(collectionMenuWorldInput(INVENTORY_MENU_B, false, LureZone::inventory()),
                true, "B closes the root menu");

    // Steady world frames on the open collection page perform no lookups.
    test.assert(collectionMenuWorldInput(INVENTORY_MENU_B, true, LureZone::inventory()),
                true, "reopen the root menu");
    collectionMenuWorldInput(INVENTORY_MENU_DOWN, false, LureZone::inventory());
    FxReadCounter::resetFrame();
    collectionMenuWorldInput(INVENTORY_MENU_A, false, LureZone::inventory());
    test.assert(FxReadCounter::count(), static_cast<uint8_t>(1),
                "opening the list resolves only its one visible name");
    for (uint8_t frame = 0; frame < 3; ++frame) {
        test.assert(collectionMenuWorldInput(0, false, LureZone::inventory()), true,
                    "open list owns every steady frame");
    }
    test.assert(FxReadCounter::count(), static_cast<uint8_t>(1),
                "steady world frames resolve no names");
    collectionMenuWorldDraw(LureZone::inventory());
    test.assert(FxReadCounter::count(), static_cast<uint8_t>(1),
                "steady world draw reads only cached names");
    collectionMenuWorldInput(INVENTORY_MENU_B, false, LureZone::inventory());
    collectionMenuWorldInput(INVENTORY_MENU_B, false, LureZone::inventory());
    item::inventoryClear(LureZone::inventory());

    suite.addTest(test);
}

// jp8.11.9: consuming the confirmed box-to-party choice replaces exactly one
// party slot. The outgoing member's current HP is mirrored to its own record
// first, the selected instance is assigned with its encoded HP, and only then
// is the one live party member rebuilt. Refusals are lossless and interrupted
// publications recover to the last journal-valid state.
void BoxToPartyReplacementTest(TestSuite &suite) {
    using namespace collection_menu_test_detail;
    Test test(__func__);
    Player party = Player();
    SaveFile state = snapshot();

    // Injured duplicate swap: exact per-instance HP/XP/moves, the outgoing
    // record is mirrored, and the remaining party slots are untouched.
    fresh();
    StoreRecord outgoing = creature(3, 126, 1 + 7);
    StoreRecord incoming = creature(3, 200, 1 + 3);
    StoreRecord other = creature(6, 150, 0);
    uint8_t sOut = 255, sIn = 255, sOther = 255;
    test.assert(storeAdd(outgoing, sOut), true, "outgoing duplicate added");
    test.assert(storeAdd(incoming, sIn), true, "incoming duplicate added");
    test.assert(storeAdd(other, sOther), true, "other party member added");
    test.assert(sOut, static_cast<uint8_t>(0), "outgoing stable slot");
    test.assert(sIn, static_cast<uint8_t>(1), "incoming stable slot");
    const uint8_t otherHP = fullHP(other);
    test.assert(storeAssign(0, sOut, 7), true, "outgoing assigned with current HP");
    test.assert(storeAssign(1, sOther, otherHP), true, "other member assigned");
    party.party[0].load(outgoing);
    party.creatureHPs[0] = 7;
    party.party[1].load(other);
    party.creatureHPs[1] = otherHP;
    party.party[2].id = 255;
    party.creatureHPs[2] = 0;
    const uint16_t journalBefore = journalCount();
    chooseSlot(sIn, 0);
    test.assert(collectionMenuWorldChoice().slot, sIn, "choice carries the duplicate slot");
    test.assert(static_cast<uint8_t>(collectionMenuWorldFeedback()),
                static_cast<uint8_t>(CollectionFeedback::None),
                "no feedback before the world consumes the choice");
    test.assert(collectionMenuWorldApplyChoice(party), true, "swap choice consumed");
    test.assert(static_cast<uint8_t>(collectionMenuWorldFeedback()),
                static_cast<uint8_t>(CollectionFeedback::None),
                "successful swap is silent");
    test.assert(journalCount(), static_cast<uint16_t>(journalBefore + 2),
                "outgoing mirror then PartyAssign published");
    test.assert(storePartySlot(0), sIn, "selected duplicate is assigned");
    test.assert(storePartyHP(0), static_cast<uint8_t>(3), "incoming assignment HP");
    test.assert(party.party[0].id, static_cast<uint8_t>(3), "incoming species rebuilt");
    test.assert(party.party[0].level, levelFromExp(200), "incoming XP-derived level");
    test.assert(party.party[0].attackMask, incoming.attackMask,
                "incoming moves preserved");
    test.assert(party.creatureHPs[0], static_cast<uint8_t>(3), "incoming exact HP installed");
    test.assert(storePartySlot(1), sOther, "other party slot not reordered");
    test.assert(storePartyHP(1), otherHP, "other party HP not changed");
    test.assert(party.party[1].id, static_cast<uint8_t>(6), "other species unchanged");
    test.assert(party.creatureHPs[1], otherHP, "other live HP unchanged");
    test.assert(storePartySlot(2), static_cast<uint8_t>(255), "empty slot stays empty");
    StoreRecord read = {};
    test.assert(storeRead(sOut, read), true, "outgoing record readable");
    test.assert(storeInstanceHP(read), static_cast<uint8_t>(7), "outgoing HP mirrored");
    test.assert(read.exp, static_cast<uint16_t>(126), "outgoing XP preserved");
    test.assert(read.attackMask, outgoing.attackMask, "outgoing moves preserved");
    test.assert(storeRead(sIn, read), true, "incoming record readable");
    test.assert(storeInstanceHP(read), static_cast<uint8_t>(3), "incoming record HP untouched");
    test.assert(read.exp, static_cast<uint16_t>(200), "incoming record XP untouched");
    FxReadCounter::resetFrame();
    for (uint8_t frame = 0; frame < 3; ++frame)
        test.assert(collectionMenuWorldInput(0, false, LureZone::inventory()), true,
                    "post-swap list owns steady frames");
    test.assert(FxReadCounter::count(), static_cast<uint8_t>(0),
                "post-swap steady frames resolve no names");
    CollectionMenu feedbackPage = {};
    collectionMenuOpen(feedbackPage);
    feedbackPage.feedback = static_cast<uint8_t>(CollectionFeedback::SaveNeeded);
    FxReadCounter::resetFrame();
    drawCollectionMenu(feedbackPage);
    test.assert(FxReadCounter::count(), static_cast<uint8_t>(0),
                "save-needed feedback draws from the row cache only");
    closeWorldMenu();

    // Empty target slot: no outgoing pub, legacy zero HP derives full, and only
    // the PartyAssign journal record is published.
    fresh();
    StoreRecord legacy = creature(6, 27, 0);
    uint8_t sLegacy = 255;
    test.assert(storeAdd(legacy, sLegacy), true, "legacy instance added");
    party = Player();
    party.party[2].id = 255;
    party.creatureHPs[2] = 0;
    const uint16_t emptyBefore = journalCount();
    chooseSlot(sLegacy, 2);
    test.assert(collectionMenuWorldApplyChoice(party), true, "empty-slot swap consumed");
    const uint8_t legacyFull = fullHP(legacy);
    test.assert(journalCount(), static_cast<uint16_t>(emptyBefore + 1),
                "empty target publishes only PartyAssign");
    test.assert(storePartySlot(2), sLegacy, "legacy instance assigned");
    test.assert(storePartyHP(2), legacyFull, "legacy zero derives full assignment HP");
    test.assert(party.party[2].id, static_cast<uint8_t>(6), "legacy member rebuilt");
    test.assert(party.creatureHPs[2], legacyFull, "legacy member keeps exact full HP");

    // KO outgoing and KO incoming: neither instance is revived.
    fresh();
    StoreRecord koOut = creature(0, 126, 1);
    StoreRecord koIn = creature(0, 126, 1);
    uint8_t sKoOut = 255, sKoIn = 255;
    test.assert(storeAdd(koOut, sKoOut), true, "KO outgoing added");
    test.assert(storeAdd(koIn, sKoIn), true, "KO incoming added");
    test.assert(storeAssign(2, sKoOut, 0), true, "KO outgoing assigned");
    party = Player();
    party.party[2].load(koOut);
    party.creatureHPs[2] = 0;
    chooseSlot(sKoIn, 2);
    test.assert(collectionMenuWorldApplyChoice(party), true, "KO swap consumed");
    test.assert(storePartySlot(2), sKoIn, "KO incoming assigned");
    test.assert(storePartyHP(2), static_cast<uint8_t>(0), "KO incoming keeps zero HP");
    test.assert(party.creatureHPs[2], static_cast<uint8_t>(0), "live KO incoming not revived");
    test.assert(storeRead(sKoOut, read), true, "KO outgoing record readable");
    test.assert(storeInstanceHP(read), static_cast<uint8_t>(0), "KO outgoing record stays KO");
    test.assert(storeRead(sKoIn, read), true, "KO incoming record readable");
    test.assert(storeInstanceHP(read), static_cast<uint8_t>(0), "KO incoming record stays KO");

    // Selecting an instance already in any party slot reports InParty without
    // mutation or reordering.
    fresh();
    StoreRecord member = creature(4, 126, 1 + 9);
    uint8_t sMember = 255;
    test.assert(storeAdd(member, sMember), true, "assigned instance added");
    test.assert(storeAssign(1, sMember, 9), true, "instance assigned to slot 2");
    party = Player();
    party.party[1].load(member);
    party.creatureHPs[1] = 9;
    party.party[0].id = 255;
    party.creatureHPs[0] = 0;
    const uint16_t assignedBefore = journalCount();
    chooseSlot(sMember, 0);
    test.assert(collectionMenuWorldApplyChoice(party), true, "already-assigned consumed");
    test.assert(static_cast<uint8_t>(collectionMenuWorldFeedback()),
                static_cast<uint8_t>(CollectionFeedback::InParty),
                "already in party is reported");
    test.assert(journalCount(), assignedBefore, "already assigned publishes nothing");
    test.assert(storePartySlot(0), static_cast<uint8_t>(255), "target slot stays empty");
    test.assert(storePartySlot(1), sMember, "existing assignment is not moved");
    test.assert(storePartyHP(1), static_cast<uint8_t>(9), "existing assignment HP unchanged");
    test.assert(party.party[0].id, static_cast<uint8_t>(255), "live target stays empty");
    test.assert(party.creatureHPs[0], static_cast<uint8_t>(0), "empty target HP stays zero");
    test.assert(party.party[1].id, static_cast<uint8_t>(4), "live existing member unchanged");
    test.assert(party.creatureHPs[1], static_cast<uint8_t>(9), "live existing HP unchanged");

    // Dirty tail: the outgoing mirror cannot publish, so the party is untouched
    // and explicit save-needed feedback is reported.
    fresh();
    StoreRecord dirtyOut = creature(3, 126, 1 + 7);
    StoreRecord dirtyIn = creature(3, 200, 1 + 3);
    uint8_t sDirtyOut = 255, sDirtyIn = 255;
    test.assert(storeAdd(dirtyOut, sDirtyOut), true, "dirty outgoing added");
    test.assert(storeAdd(dirtyIn, sDirtyIn), true, "dirty incoming added");
    test.assert(storeAssign(0, sDirtyOut, 7), true, "dirty outgoing assigned");
    party = Player();
    party.party[0].load(dirtyOut);
    party.creatureHPs[0] = 2;
    flashFakeSetWriteLimit(15);
    test.assert(storeWrite(63, creature(9, 1)), false, "torn record leaves a dirty tail");
    flashFakeSetWriteLimit(-1);
    const uint16_t dirtyJournal = journalCount();
    chooseSlot(sDirtyIn, 0);
    test.assert(collectionMenuWorldApplyChoice(party), true, "dirty-tail swap consumed");
    test.assert(static_cast<uint8_t>(collectionMenuWorldFeedback()),
                static_cast<uint8_t>(CollectionFeedback::SaveNeeded),
                "dirty tail reports save needed");
    test.assert(journalCount(), dirtyJournal, "dirty tail publishes no mirror");
    test.assert(storePartySlot(0), sDirtyOut, "dirty tail keeps the outgoing member");
    test.assert(storePartyHP(0), static_cast<uint8_t>(7), "dirty tail keeps assignment HP");
    test.assert(party.party[0].id, static_cast<uint8_t>(3), "dirty tail keeps live species");
    test.assert(party.creatureHPs[0], static_cast<uint8_t>(2), "dirty tail keeps live HP");
    test.assert(storeRead(sDirtyOut, read), true, "dirty outgoing record readable");
    test.assert(storeInstanceHP(read), static_cast<uint8_t>(7),
                "unpublished mirror leaves the old record HP");

    // Full pending index: a non-pending outgoing slot refuses before any write.
    fresh();
    StoreRecord pendingOut = creature(3, 126, 1 + 7);
    StoreRecord pendingIn = creature(3, 200, 1 + 3);
    uint8_t sPendingOut = 255, sPendingIn = 255;
    test.assert(storeAdd(pendingOut, sPendingOut), true, "pending outgoing added");
    test.assert(storeAdd(pendingIn, sPendingIn), true, "pending incoming added");
    test.assert(storeAssign(0, sPendingOut, 7), true, "pending outgoing assigned");
    test.assert(save(state), SaveStep::Done, "pending-index baseline commits");
    party = Player();
    party.party[0].load(pendingOut);
    party.creatureHPs[0] = 2;
    for (uint8_t i = 0; i < 16; ++i)
        test.assert(storeWrite(static_cast<uint8_t>(10 + i),
                               creature(static_cast<uint8_t>(10 + i), 27)),
                    true, "pending index fills");
    const uint16_t fullJournal = journalCount();
    chooseSlot(sPendingIn, 0);
    test.assert(collectionMenuWorldApplyChoice(party), true, "full-index swap consumed");
    test.assert(static_cast<uint8_t>(collectionMenuWorldFeedback()),
                static_cast<uint8_t>(CollectionFeedback::SaveNeeded),
                "full pending index reports save needed");
    test.assert(journalCount(), fullJournal, "full index publishes nothing");
    test.assert(storePartySlot(0), sPendingOut, "full index keeps the outgoing member");
    test.assert(storePartyHP(0), static_cast<uint8_t>(7), "full index keeps assignment HP");
    test.assert(party.creatureHPs[0], static_cast<uint8_t>(2), "full index keeps live HP");
    test.assert(storeRead(sPendingOut, read), true, "full-index outgoing record readable");
    test.assert(storeInstanceHP(read), static_cast<uint8_t>(7),
                "full index leaves the old record HP");

    // An active save refuses the assignment even with an empty target slot.
    fresh();
    StoreRecord busy = creature(6, 27, 0);
    uint8_t sBusy = 255;
    test.assert(storeAdd(busy, sBusy), true, "busy-save instance added");
    party = Player();
    party.party[0].id = 255;
    party.creatureHPs[0] = 0;
    const uint16_t busyJournal = journalCount();
    saveBegin();
    chooseSlot(sBusy, 0);
    test.assert(collectionMenuWorldApplyChoice(party), true, "busy-save swap consumed");
    test.assert(static_cast<uint8_t>(collectionMenuWorldFeedback()),
                static_cast<uint8_t>(CollectionFeedback::SaveNeeded),
                "active save reports save needed");
    test.assert(journalCount(), busyJournal, "active save publishes nothing");
    test.assert(storePartySlot(0), static_cast<uint8_t>(255), "active save keeps target empty");
    test.assert(party.party[0].id, static_cast<uint8_t>(255), "active save keeps live empty");
    test.assert(party.creatureHPs[0], static_cast<uint8_t>(0), "active save keeps live HP zero");
    storeFresh();

    // A cut between the outgoing mirror and PartyAssign recovers to the last
    // published valid state and can retry after an explicit save.
    fresh();
    StoreRecord cutOut = creature(3, 126, 1 + 7);
    StoreRecord cutIn = creature(3, 200, 1 + 3);
    uint8_t sCutOut = 255, sCutIn = 255;
    test.assert(storeAdd(cutOut, sCutOut), true, "cut outgoing added");
    test.assert(storeAdd(cutIn, sCutIn), true, "cut incoming added");
    test.assert(storeAssign(0, sCutOut, 7), true, "cut outgoing assigned");
    test.assert(save(state), SaveStep::Done, "cut baseline commits");
    party = Player();
    party.party[0].load(cutOut);
    party.creatureHPs[0] = 2;
    chooseSlot(sCutIn, 0);
    flashFakeSetWriteLimit(20);
    test.assert(collectionMenuWorldApplyChoice(party), true, "cut swap consumed");
    flashFakeSetWriteLimit(-1);
    test.assert(static_cast<uint8_t>(collectionMenuWorldFeedback()),
                static_cast<uint8_t>(CollectionFeedback::SaveNeeded),
                "cut reports save needed");
    test.assert(journalCount(), static_cast<uint16_t>(1),
                "only the outgoing mirror committed before the cut");
    test.assert(storePartySlot(0), sCutOut, "cut keeps the outgoing member");
    test.assert(party.party[0].id, static_cast<uint8_t>(3), "cut keeps live species");
    test.assert(party.creatureHPs[0], static_cast<uint8_t>(2), "cut keeps live HP");
    test.assert(storeRead(sCutOut, read), true, "cut outgoing record readable");
    test.assert(storeInstanceHP(read), static_cast<uint8_t>(2),
                "cut publishes the outgoing mirror");
    SaveFile loaded = snapshot();
    test.assert(storeBoot(loaded), true, "cut reboots");
    test.assert(storeBootFailed(), false, "cut does not fail closed");
    test.assert(loaded.partySlots[0], sCutOut, "cut recovery keeps the outgoing slot");
    test.assert(loaded.partyHP[0], static_cast<uint8_t>(2),
                "cut recovery matches the mirrored HP");
    test.assert(storeRead(sCutOut, read), true, "cut recovered outgoing readable");
    test.assert(storeInstanceHP(read), static_cast<uint8_t>(2), "cut recovered outgoing HP");
    test.assert(storeRead(sCutIn, read), true, "cut recovered incoming readable");
    test.assert(storeInstanceHP(read), static_cast<uint8_t>(3),
                "cut leaves the incoming record untouched");
    test.assert(save(state), SaveStep::Done, "cut dirty tail drains");
    chooseSlot(sCutIn, 0);
    test.assert(collectionMenuWorldApplyChoice(party), true, "cut retry consumed");
    test.assert(static_cast<uint8_t>(collectionMenuWorldFeedback()),
                static_cast<uint8_t>(CollectionFeedback::None), "cut retry succeeds");
    test.assert(storePartySlot(0), sCutIn, "cut retry assigns the incoming member");
    test.assert(storePartyHP(0), static_cast<uint8_t>(3), "cut retry installs incoming HP");
    test.assert(party.party[0].id, static_cast<uint8_t>(3), "cut retry rebuilds species");
    test.assert(party.creatureHPs[0], static_cast<uint8_t>(3), "cut retry installs exact HP");
    closeWorldMenu();

    suite.addTest(test);
}

void CollectionMenuSuite(TestRunner &runner) {
    TestSuite suite("Collection Menu");
    CollectionMenuTest(suite);
    BoxToPartyReplacementTest(suite);
    runner.addTestSuite(suite);
}
