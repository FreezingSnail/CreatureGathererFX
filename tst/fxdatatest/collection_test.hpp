#pragma once
// Device proof for the four-row stable-slot collection view and the world
// BAG/COLLECTION root menu (jp8.11.8). Complements the host suite: this one
// runs the real v3 save store, the real FX name-address lookups and the
// metadata-read counter on AVR.
//
// Wired by tst/fxdatatest/test_collection.ino.

#include <string.h>

#include "fxtest.hpp"
#include "src/creature/LevelCurve.hpp"
#include "src/engine/menu/CollectionMenu.hpp"
#include "src/engine/menu/InventoryMenu.hpp"
#include "src/engine/world/LureZone.hpp"
#include "src/fxdata.h"
#include "src/item/Inventory.hpp"
#include "src/lib/FxReadCounter.hpp"
#include "src/lib/ReadData.hpp"
#include "src/player/Player.hpp"
#include "src/save/Compaction.hpp"
#include "src/save/FlashBackend.hpp"
#include "src/save/Journal.hpp"
#include "src/save/SaveFile.hpp"

namespace collection_fx_test_detail {

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

// Fixtures in this suite allocate contiguous stable slots from zero, so the
// visible row equals the stable slot.
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
    for (uint16_t i = 0; i < 256 && saveInProgress(); ++i) {
        result = saveStepAdvance(state);
        FX::waitWhileBusy();
    }
    return result;
}

// Both v3 checkpoint sectors are cleared before each scenario so storeFresh
// starts from erased storage instead of another suite's committed checkpoint.
void eraseSaves() {
    const uint16_t pages[] = {32, 48};
    for (uint8_t i = 0; i < sizeof(pages) / sizeof(pages[0]); ++i) {
        flash.eraseSector(pages[i]);
        while (flash.busy()) {}
    }
}

void fresh() {
    eraseSaves();
    storeFresh();
}

} // namespace collection_fx_test_detail

void test_collection(FxTest &test) {
    using namespace collection_fx_test_detail;
    CollectionMenu menu = {};

    // Empty store: no rows, no name lookups, no selection.
    fresh();
    FxReadCounter::resetFrame();
    collectionMenuOpen(menu);
    test.expectEq(menu.view.itemCount, 0, F("empty collection has no rows"));
    test.expectEq(menu.rowSlot[0], 255, F("empty row cache is clear"));
    test.expectEq(FxReadCounter::count(), 0, F("empty open resolves no names"));
    test.expectEq(collectionMenuSelect(menu), 255, F("empty collection selects nothing"));
    drawCollectionMenu(menu);
    test.expectEq(FxReadCounter::count(), 0, F("empty draw resolves no names"));

    // Duplicate species own separate rows; a tombstone is skipped; the party
    // marker follows the stable slot.
    fresh();
    StoreRecord first = creature(3, 27);
    StoreRecord duplicate = creature(3, 125);
    StoreRecord removed = creature(5, 8);
    uint8_t slot0 = 255;
    uint8_t slot1 = 255;
    uint8_t slot2 = 255;
    test.expectEq(storeAdd(first, slot0), true, F("first duplicate accepted"));
    test.expectEq(storeAdd(duplicate, slot1), true, F("duplicate species accepted"));
    test.expectEq(storeAdd(removed, slot2), true, F("tombstone candidate accepted"));
    test.expectEq(storeRemove(slot2), true, F("candidate tombstoned"));
    test.expectEq(storeAssign(0, slot0, 0), true, F("party assignment accepted"));
    FxReadCounter::resetFrame();
    collectionMenuOpen(menu);
    test.expectEq(menu.view.itemCount, 2, F("tombstone is not counted"));
    test.expectEq(menu.rowSlot[0], slot0, F("first stable slot"));
    test.expectEq(menu.rowSlot[1], slot1, F("duplicate keeps its own slot"));
    test.expectEq(menu.rowId[0], 3, F("first row species"));
    test.expectEq(menu.rowId[1], 3, F("duplicate is not deduped"));
    test.expectEq(menu.rowLevel[0], levelFromExp(27), F("level derives from exp"));
    test.expectEq(menu.rowLevel[1], levelFromExp(125), F("duplicate level derives"));
    test.expectEq(menu.rowWidth[0], readCreatureNameWidth(3), F("cached name width"));
    test.expectEq(menu.rowParty, 1, F("only the assigned slot marks party"));
    test.expectEq(FxReadCounter::count(), 2, F("open resolves two visible names"));
    const uint8_t opened = FxReadCounter::count();
    drawCollectionMenu(menu);
    test.expectEq(FxReadCounter::count(), opened, F("draw uses cached names"));
    collectionMenuMove(menu, 1);
    test.expectEq(FxReadCounter::count(), opened, F("in-window move reads nothing"));
    test.expectEq(collectionMenuSelect(menu), slot1, F("selection carries the stable slot"));

    // Eight occupied slots: scrolling refreshes exactly one window and stops
    // at both ends.
    fresh();
    for (uint8_t i = 0; i < 8; ++i) {
        StoreRecord row = creature(static_cast<uint8_t>(10 + i),
                                   levelExpThreshold(1) + i);
        uint8_t slot = 255;
        storeAdd(row, slot);
    }
    FxReadCounter::resetFrame();
    collectionMenuOpen(menu);
    test.expectEq(menu.view.itemCount, 8, F("eight occupied rows"));
    test.expectEq(menu.rowSlot[3], 3, F("first window ends at slot 3"));
    test.expectEq(FxReadCounter::count(), 4, F("open resolves four names"));
    collectionMenuMove(menu, 100);
    test.expectEq(menu.view.cursor, 7, F("down clamps at the last row"));
    test.expectEq(menu.view.windowStart, 4, F("end window holds four rows"));
    test.expectEq(menu.rowSlot[0], 4, F("scrolled window starts at slot 4"));
    test.expectEq(menu.rowSlot[3], 7, F("scrolled window ends at slot 7"));
    test.expectEq(FxReadCounter::count(), 8, F("scroll resolves one window"));
    collectionMenuMove(menu, 1);
    test.expectEq(menu.view.cursor, 7, F("down at the end does not wrap"));
    collectionMenuMove(menu, -100);
    test.expectEq(menu.view.cursor, 0, F("up clamps at the first row"));
    test.expectEq(collectionMenuSelect(menu), 0, F("first selection is slot 0"));

    // Last stable slot 63 keeps its identity.
    fresh();
    test.expectEq(storeWrite(63, creature(9, 1)), true, F("slot 63 accepts an instance"));
    collectionMenuOpen(menu);
    test.expectEq(menu.view.itemCount, 1, F("slot 63 is listed"));
    test.expectEq(menu.rowSlot[0], 63, F("slot 63 keeps its identity"));
    test.expectEq(collectionMenuSelect(menu), 63, F("slot 63 selection stays 63"));

    // Full 64-slot collection survives explicit checkpoints and still reaches
    // slot 63 through four rows.
    fresh();
    SaveFile state = snapshot();
    uint8_t lastSlot = 255;
    for (uint8_t i = 0; i < 64; ++i) {
        StoreRecord row = creature(static_cast<uint8_t>(i % 32),
                                   levelExpThreshold(1) + i);
        uint8_t slot = 255;
        test.expectEq(storeAdd(row, slot), true, F("full-collection add accepted"));
        test.expectEq(slot, i, F("stable slot allocation"));
        lastSlot = slot;
        if ((i & 15) == 15)
            test.expectEq(static_cast<uint8_t>(save(state)),
                          static_cast<uint8_t>(SaveStep::Done),
                          F("batch checkpoint completes"));
    }
    test.expectEq(storeAdd(creature(0, 1), lastSlot), false,
                  F("full64 refuses an add"));
    FxReadCounter::resetFrame();
    collectionMenuOpen(menu);
    test.expectEq(menu.view.itemCount, 64, F("full collection lists 64 rows"));
    test.expectEq(menu.rowSlot[3], 3, F("full first window holds slots 0..3"));
    test.expectEq(FxReadCounter::count(), 4, F("full open resolves four names"));
    collectionMenuMove(menu, 100);
    test.expectEq(menu.view.cursor, 63, F("full collection reaches the last row"));
    test.expectEq(menu.view.windowStart, 60, F("last window starts at row 60"));
    test.expectEq(menu.rowSlot[3], 63, F("last window reaches slot 63"));
    test.expectEq(collectionMenuSelect(menu), 63, F("last selection is slot 63"));
    test.expectEq(FxReadCounter::count(), 8, F("full scroll resolves one window"));
    drawCollectionMenu(menu);
    test.expectEq(FxReadCounter::count(), 8, F("full draw reads cached names"));

    // World root menu: BAG/COLLECTION/SAVE, fresh edges only, B closes, and the
    // collection page owns every steady frame without metadata reads.
    fresh();
    item::inventoryClear(LureZone::inventory());
    item::inventoryAdd(LureZone::inventory(), item::ItemKind::Lure,
                       item::lureId(1, 0), 1);
    if (inventoryMenuWorldOpen())
        inventoryMenuWorldInput(INVENTORY_MENU_B, true, LureZone::inventory());
    test.expectEq(collectionMenuWorldInput(INVENTORY_MENU_B, false,
                                           LureZone::inventory()),
                  false, F("busy world cannot open the menu"));
    test.expectEq(collectionMenuWorldOpen(), false, F("busy world leaves it closed"));
    test.expectEq(collectionMenuWorldInput(INVENTORY_MENU_B, true,
                                           LureZone::inventory()),
                  true, F("fresh B opens the root menu"));
    test.expectEq(static_cast<uint8_t>(collectionMenuWorldPage()),
                  static_cast<uint8_t>(CollectionPage::Root), F("root page"));
    test.expectEq(collectionMenuWorldChoice().slot, 255, F("open clears the choice"));
    test.expectEq(collectionMenuWorldInput(INVENTORY_MENU_A, false,
                                           LureZone::inventory()),
                  true, F("A opens BAG"));
    test.expectEq(inventoryMenuWorldOpen(), true, F("bag page opens"));
    test.expectEq(collectionMenuWorldInput(INVENTORY_MENU_B, false,
                                           LureZone::inventory()),
                  true, F("B closes BAG"));
    test.expectEq(static_cast<uint8_t>(collectionMenuWorldPage()),
                  static_cast<uint8_t>(CollectionPage::Root),
                  F("B returns to root"));
    test.expectEq(collectionMenuWorldInput(INVENTORY_MENU_DOWN, false,
                                           LureZone::inventory()),
                  true, F("down moves to COLLECTION"));
    test.expectEq(collectionMenuWorldInput(INVENTORY_MENU_DOWN, false,
                                           LureZone::inventory()),
                  true, F("down moves to SAVE"));
    test.expectEq(collectionMenuWorldInput(INVENTORY_MENU_DOWN, false,
                                           LureZone::inventory()),
                  true, F("down clamps at SAVE"));
    test.expectEq(collectionMenuWorldInput(INVENTORY_MENU_UP, false,
                                           LureZone::inventory()),
                  true, F("up returns to COLLECTION"));
    test.expectEq(collectionMenuWorldInput(INVENTORY_MENU_A, false,
                                           LureZone::inventory()),
                  true, F("A opens the collection"));
    test.expectEq(static_cast<uint8_t>(collectionMenuWorldPage()),
                  static_cast<uint8_t>(CollectionPage::List), F("list page"));
    test.expectEq(collectionMenuWorldInput(INVENTORY_MENU_A, false,
                                           LureZone::inventory()),
                  true, F("A on an empty collection stays put"));
    test.expectEq(static_cast<uint8_t>(collectionMenuWorldPage()),
                  static_cast<uint8_t>(CollectionPage::List), F("empty list page"));
    uint8_t worldSlot = 255;
    test.expectEq(storeAdd(creature(6, levelExpThreshold(3)), worldSlot), true,
                  F("world fixture accepted"));
    // Reopen the collection so the newly added instance is visible.
    collectionMenuWorldInput(INVENTORY_MENU_B, false, LureZone::inventory());
    FxReadCounter::resetFrame();
    test.expectEq(collectionMenuWorldInput(INVENTORY_MENU_A, false,
                                           LureZone::inventory()),
                  true, F("A reopens the collection"));
    test.expectEq(FxReadCounter::count(), 1, F("list open resolves one name"));
    test.expectEq(collectionMenuWorldInput(INVENTORY_MENU_A, false,
                                           LureZone::inventory()),
                  true, F("A opens party-slot selection"));
    test.expectEq(static_cast<uint8_t>(collectionMenuWorldPage()),
                  static_cast<uint8_t>(CollectionPage::Party), F("party page"));
    test.expectEq(collectionMenuWorldInput(INVENTORY_MENU_B, false,
                                           LureZone::inventory()),
                  true, F("B cancels party selection"));
    test.expectEq(static_cast<uint8_t>(collectionMenuWorldPage()),
                  static_cast<uint8_t>(CollectionPage::List), F("cancel returns to list"));
    test.expectEq(collectionMenuWorldChoice().slot, 255, F("cancel commits no choice"));
    collectionMenuWorldInput(INVENTORY_MENU_A, false, LureZone::inventory());
    test.expectEq(collectionMenuWorldInput(INVENTORY_MENU_DOWN, false,
                                           LureZone::inventory()),
                  true, F("down selects party slot 2"));
    test.expectEq(collectionMenuWorldInput(INVENTORY_MENU_A, false,
                                           LureZone::inventory()),
                  true, F("A confirms the party choice"));
    test.expectEq(collectionMenuWorldChoice().slot, worldSlot,
                  F("choice carries the stable slot"));
    test.expectEq(collectionMenuWorldChoice().partySlot, 2, F("choice carries party slot 2"));
    FxReadCounter::resetFrame();
    for (uint8_t frame = 0; frame < 3; ++frame)
        test.expectEq(collectionMenuWorldInput(0, false, LureZone::inventory()), true,
                      F("list owns steady frames"));
    test.expectEq(FxReadCounter::count(), 0, F("steady frames read no names"));
    collectionMenuWorldDraw(LureZone::inventory());
    test.expectEq(FxReadCounter::count(), 0, F("steady draw reads no names"));
    collectionMenuWorldInput(INVENTORY_MENU_B, false, LureZone::inventory());
    test.expectEq(static_cast<uint8_t>(collectionMenuWorldPage()),
                  static_cast<uint8_t>(CollectionPage::Root), F("root after list B"));
    collectionMenuWorldInput(INVENTORY_MENU_B, false, LureZone::inventory());
    test.expectEq(collectionMenuWorldOpen(), false, F("root B closes the menu"));
    item::inventoryClear(LureZone::inventory());

    // jp8.11.9: consuming the confirmed choice replaces exactly one party slot
    // on the real AVR store. The outgoing member's current HP is mirrored to its
    // own record first, then the selected duplicate is assigned its encoded HP.
    fresh();
    player = Player();
    StoreRecord swapOut = creature(3, 126, 8);  // 7 current HP
    StoreRecord swapIn = creature(3, 200, 4);   // duplicate species, 3 HP
    StoreRecord swapOther = creature(6, 150, 0);
    uint8_t swapOutSlot = 255, swapInSlot = 255, swapOtherSlot = 255;
    test.expectEq(storeAdd(swapOut, swapOutSlot), true, F("swap outgoing added"));
    test.expectEq(storeAdd(swapIn, swapInSlot), true, F("swap incoming added"));
    test.expectEq(storeAdd(swapOther, swapOtherSlot), true, F("swap other added"));
    const uint8_t swapOtherHP = fullHP(swapOther);
    test.expectEq(storeAssign(0, swapOutSlot, 7), true, F("swap outgoing assigned"));
    test.expectEq(storeAssign(1, swapOtherSlot, swapOtherHP), true, F("swap other assigned"));
    player.party[0].load(swapOut);
    player.creatureHPs[0] = 7;
    player.party[1].load(swapOther);
    player.creatureHPs[1] = swapOtherHP;
    player.party[2].id = 255;
    player.creatureHPs[2] = 0;
    chooseSlot(swapInSlot, 0);
    test.expectEq(collectionMenuWorldApplyChoice(player), true, F("swap choice consumed"));
    test.expectEq(static_cast<uint8_t>(collectionMenuWorldFeedback()),
                  static_cast<uint8_t>(CollectionFeedback::None),
                  F("swap succeeds without feedback"));
    test.expectEq(storePartySlot(0), swapInSlot, F("swap assigns incoming slot"));
    test.expectEq(storePartyHP(0), 3, F("swap installs incoming HP"));
    test.expectEq(player.party[0].id, 3, F("swap rebuilds incoming species"));
    test.expectEq(player.party[0].level, levelFromExp(200), F("swap rebuilds incoming level"));
    test.expectEq(player.party[0].attackMask, swapIn.attackMask, F("swap preserves moves"));
    test.expectEq(player.creatureHPs[0], 3, F("swap installs exact live HP"));
    test.expectEq(storePartySlot(1), swapOtherSlot, F("swap leaves other slot"));
    test.expectEq(storePartyHP(1), swapOtherHP, F("swap leaves other HP"));
    test.expectEq(player.party[1].id, 6, F("swap leaves other species"));
    test.expectEq(player.creatureHPs[1], swapOtherHP, F("swap leaves other live HP"));
    StoreRecord swapRead = {};
    test.expectEq(storeRead(swapOutSlot, swapRead), true, F("swap outgoing readable"));
    test.expectEq(storeInstanceHP(swapRead), 7, F("swap mirrors outgoing HP"));
    test.expectEq(swapRead.exp, 126, F("swap preserves outgoing XP"));
    test.expectEq(swapRead.attackMask, swapOut.attackMask, F("swap preserves outgoing moves"));
    test.expectEq(storeRead(swapInSlot, swapRead), true, F("swap incoming readable"));
    test.expectEq(storeInstanceHP(swapRead), 3, F("swap leaves incoming record HP"));
    test.expectEq(swapRead.exp, 200, F("swap leaves incoming record XP"));

    // Selecting an instance already in any party slot is a no-op refusal.
    fresh();
    player = Player();
    StoreRecord assigned = creature(4, 126, 10);  // 9 HP
    uint8_t assignedSlot = 255;
    test.expectEq(storeAdd(assigned, assignedSlot), true, F("assigned instance added"));
    test.expectEq(storeAssign(1, assignedSlot, 9), true, F("assigned instance placed"));
    player.party[1].load(assigned);
    player.creatureHPs[1] = 9;
    player.party[0].id = 255;
    player.creatureHPs[0] = 0;
    const uint16_t assignedJournal = journalCount();
    chooseSlot(assignedSlot, 0);
    test.expectEq(collectionMenuWorldApplyChoice(player), true,
                  F("already-assigned choice consumed"));
    test.expectEq(static_cast<uint8_t>(collectionMenuWorldFeedback()),
                  static_cast<uint8_t>(CollectionFeedback::InParty),
                  F("already-assigned feedback"));
    test.expectEq(journalCount(), assignedJournal, F("already assigned publishes nothing"));
    test.expectEq(storePartySlot(0), 255, F("already assigned leaves target empty"));
    test.expectEq(storePartySlot(1), assignedSlot, F("already assigned keeps its party slot"));
    test.expectEq(storePartyHP(1), 9, F("already assigned keeps its HP"));
    test.expectEq(player.party[0].id, 255, F("already assigned live target unchanged"));
    test.expectEq(player.creatureHPs[0], 0, F("already assigned live target HP unchanged"));
    test.expectEq(player.party[1].id, 4, F("already assigned live member unchanged"));
    test.expectEq(player.creatureHPs[1], 9, F("already assigned live HP unchanged"));

    // Empty target and legacy zero HP: the encoded record derives full HP and
    // only the PartyAssign record is published.
    fresh();
    player = Player();
    StoreRecord legacy = creature(6, 27, 0);
    uint8_t legacySlot = 255;
    test.expectEq(storeAdd(legacy, legacySlot), true, F("legacy instance added"));
    player.party[2].id = 255;
    player.creatureHPs[2] = 0;
    const uint16_t legacyJournal = journalCount();
    chooseSlot(legacySlot, 2);
    test.expectEq(collectionMenuWorldApplyChoice(player), true, F("legacy swap consumed"));
    const uint8_t legacyFull = fullHP(legacy);
    test.expectEq(journalCount(), legacyJournal + 1, F("legacy swap publishes only assign"));
    test.expectEq(storePartySlot(2), legacySlot, F("legacy instance assigned"));
    test.expectEq(storePartyHP(2), legacyFull, F("legacy zero derives full party HP"));
    test.expectEq(player.party[2].id, 6, F("legacy member rebuilt"));
    test.expectEq(player.creatureHPs[2], legacyFull, F("legacy member gets full live HP"));

    // KO outgoing and KO incoming stay KO; the swap never revives either one.
    fresh();
    player = Player();
    StoreRecord koOut = creature(0, 126, 1);
    StoreRecord koIn = creature(0, 126, 1);
    uint8_t koOutSlot = 255, koInSlot = 255;
    test.expectEq(storeAdd(koOut, koOutSlot), true, F("KO outgoing added"));
    test.expectEq(storeAdd(koIn, koInSlot), true, F("KO incoming added"));
    test.expectEq(storeAssign(2, koOutSlot, 0), true, F("KO outgoing assigned"));
    player.party[2].load(koOut);
    player.creatureHPs[2] = 0;
    chooseSlot(koInSlot, 2);
    test.expectEq(collectionMenuWorldApplyChoice(player), true, F("KO swap consumed"));
    test.expectEq(storePartySlot(2), koInSlot, F("KO incoming assigned"));
    test.expectEq(storePartyHP(2), 0, F("KO incoming keeps zero party HP"));
    test.expectEq(player.creatureHPs[2], 0, F("KO incoming live HP stays zero"));
    test.expectEq(storeRead(koOutSlot, swapRead), true, F("KO outgoing readable"));
    test.expectEq(storeInstanceHP(swapRead), 0, F("KO outgoing record stays KO"));
    test.expectEq(storeRead(koInSlot, swapRead), true, F("KO incoming readable"));
    test.expectEq(storeInstanceHP(swapRead), 0, F("KO incoming record stays KO"));

    // Full pending index refuses the outgoing mirror before any publication.
    fresh();
    player = Player();
    StoreRecord blockedOut = creature(3, 126, 8);
    StoreRecord blockedIn = creature(3, 200, 4);
    uint8_t blockedOutSlot = 255, blockedInSlot = 255;
    test.expectEq(storeAdd(blockedOut, blockedOutSlot), true, F("blocked outgoing added"));
    test.expectEq(storeAdd(blockedIn, blockedInSlot), true, F("blocked incoming added"));
    test.expectEq(storeAssign(0, blockedOutSlot, 7), true, F("blocked outgoing assigned"));
    SaveFile pendingState = snapshot();
    test.expectEq(static_cast<uint8_t>(save(pendingState)),
                  static_cast<uint8_t>(SaveStep::Done),
                  F("pending-index baseline commits"));
    player.party[0].load(blockedOut);
    player.creatureHPs[0] = 2;
    for (uint8_t i = 0; i < 16; ++i)
        test.expectEq(storeWrite(static_cast<uint8_t>(10 + i),
                                 creature(static_cast<uint8_t>(10 + i), 27)),
                      true, F("pending index fills"));
    const uint16_t blockedJournal = journalCount();
    chooseSlot(blockedInSlot, 0);
    test.expectEq(collectionMenuWorldApplyChoice(player), true,
                  F("blocked swap consumed"));
    test.expectEq(static_cast<uint8_t>(collectionMenuWorldFeedback()),
                  static_cast<uint8_t>(CollectionFeedback::SaveNeeded),
                  F("full index reports save needed"));
    test.expectEq(journalCount(), blockedJournal, F("full index publishes nothing"));
    test.expectEq(storePartySlot(0), blockedOutSlot, F("full index keeps outgoing slot"));
    test.expectEq(storePartyHP(0), 7, F("full index keeps assignment HP"));
    test.expectEq(player.creatureHPs[0], 2, F("full index keeps live HP"));
    test.expectEq(storeRead(blockedOutSlot, swapRead), true,
                  F("blocked outgoing readable"));
    test.expectEq(storeInstanceHP(swapRead), 7, F("full index leaves old record HP"));
    closeWorldMenu();
}
