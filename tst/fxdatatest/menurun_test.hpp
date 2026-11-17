#pragma once
// Permanent device proof for the first-beta menu metadata-read policy
// (CreatureGathererFX-jp8.4.10). Every current menu is exercised through its
// production entry point on the real FX image and the logical read counter:
// lookups are allowed only on the transition that resolves them and a steady
// frame performs none. Raster blits are not part of the counter.
//
// Scope is the shipping beta world views: the BAG/COLLECTION root and bag
// pages, the lure picker, and full-world dialogs. The duel BattleMenu page
// matrix lives in test_items.ino beside the shared item pages; arena suites
// keep their own enabled-profile counter proof.
//
// Wired by tst/fxdatatest/test_world_bag.ino.

#include "fxtest.hpp"
#include "src/engine/menu/CollectionMenu.hpp"
#include "src/engine/menu/DialogMenu.hpp"
#include "src/engine/menu/InventoryMenu.hpp"
#include "src/engine/world/LureZone.hpp"
#include "src/fxdata.h"
#include "src/item/Inventory.hpp"
#include "src/lib/FxReadCounter.hpp"
#include "src/player/Player.hpp"
#include "src/save/Compaction.hpp"
#include "src/save/FlashBackend.hpp"

namespace menurun_test_detail {

StoreRecord storedCreature(uint8_t id, uint16_t exp, uint8_t hpEncoded = 0) {
    StoreRecord record = {};
    record.id = id;
    record.exp = exp;
    record.attackMask = 2;
    record.hpEncoded = hpEncoded;
    return record;
}

// Both v3 checkpoint sectors are erased before a scenario so the store starts
// from erased storage instead of another case's committed data.
void eraseStore() {
    const uint16_t pages[] = {32, 48};
    for (uint8_t i = 0; i < sizeof(pages) / sizeof(pages[0]); ++i) {
        flash.eraseSector(pages[i]);
        while (flash.busy()) {}
    }
    storeFresh();
}

void closeWorldMenu() {
    for (uint8_t i = 0; i < 4 && collectionMenuWorldOpen(); ++i)
        collectionMenuWorldInput(INVENTORY_MENU_B, true, LureZone::inventory());
    if (inventoryMenuWorldOpen())
        inventoryMenuWorldInput(INVENTORY_MENU_B, true, LureZone::inventory());
}

// --- Full-world dialogs ----------------------------------------------------
// Push resolves the head once; queued pushes resolve nothing until the pop that
// promotes them. Drawing the resident text is raster-only. The empty script
// index is the real "no authored text" state: one bound read, width zero.
void menuRunDialogs(FxTest &test) {
    dialogMenu.clear();
    FxReadCounter::resetFrame();
    test.expectEq(dialogMenu.push(PopUpDialog{0, 34, 120, 30, creature21, TEXT}),
                  true, F("world dialog push accepted"));
    test.expectEq(FxReadCounter::count(), static_cast<uint8_t>(1),
                  F("dialog push resolves one head"));
    test.expectEq(FxReadCounter::markUpdate(), true,
                  F("dialog push is a transition"));
    for (uint8_t frame = 0; frame < 3; ++frame) dialogMenu.drawPopMenu();
    test.expectEq(FxReadCounter::count(), static_cast<uint8_t>(1),
                  F("dialog draw is raster only"));
    test.expectEq(FxReadCounter::renderExact(0), true,
                  F("dialog render reads no metadata"));

    FxReadCounter::resetFrame();
    test.expectEq(dialogMenu.push(PopUpDialog{0, 34, 120, 30, creature21, TEXT}),
                  true, F("queued dialog accepted"));
    test.expectEq(FxReadCounter::count(), static_cast<uint8_t>(0),
                  F("queued push resolves nothing"));
    dialogMenu.popMenu();
    test.expectEq(FxReadCounter::count(), static_cast<uint8_t>(1),
                  F("pop resolves the promoted head"));
    test.expectEq(FxReadCounter::markUpdate(), true,
                  F("dialog pop is a transition"));
    FxReadCounter::resetFrame();
    dialogMenu.popMenu();
    test.expectEq(dialogMenu.peek(), false, F("last dialog closes"));
    test.expectEq(FxReadCounter::count(), static_cast<uint8_t>(0),
                  F("closing the last dialog reads nothing"));
    test.expectEq(FxReadCounter::markUpdate(), true,
                  F("empty dialog close stays steady"));

    FxReadCounter::resetFrame();
    test.expectEq(dialogMenu.push(
                      PopUpDialog{0, 40, 128, 24, 0xFFFF, SCRIPT_TEXT}),
                  true, F("empty script dialog accepted"));
    test.expectEq(dialogMenu.head().width, static_cast<uint8_t>(0),
                  F("empty script index has no text"));
    test.expectEq(FxReadCounter::count(), static_cast<uint8_t>(1),
                  F("script bound resolves once"));
    test.expectEq(FxReadCounter::markUpdate(), true,
                  F("empty script is a transition"));
    dialogMenu.drawPopMenu();
    test.expectEq(FxReadCounter::count(), static_cast<uint8_t>(1),
                  F("empty script draw reads nothing"));
    dialogMenu.clear();
}

// --- Lure picker -----------------------------------------------------------
// Five visible rows, two name parts each. Open, a window scroll and a deposit
// resolve the window; in-window cursor moves, steady frames and the B close do
// not. The one-lure deposit is the real exhaustion edge.
void menuRunPicker(FxTest &test) {
    item::inventoryClear(LureZone::inventory());
    for (uint8_t id = 0; id < 6; ++id)
        item::inventoryAdd(LureZone::inventory(), item::ItemKind::Lure, id, 1);
    item::inventoryAdd(LureZone::inventory(), item::ItemKind::Consumable, 0, 1);
    LureZone::closePicker();
    LureZone::clear();

    FxReadCounter::resetFrame();
    LureZone::openPicker();
    test.expectEq(LureZone::pickerCount(), static_cast<uint8_t>(6),
                  F("picker lists only the six lures"));
    test.expectEq(FxReadCounter::count(), static_cast<uint8_t>(10),
                  F("picker open resolves five lure rows"));
    test.expectEq(FxReadCounter::markUpdate(), true,
                  F("picker open is a transition"));
    for (uint8_t frame = 0; frame < 3; ++frame) {
        LureZone::pickerInput(0);
        LureZone::drawOverlay();
    }
    test.expectEq(FxReadCounter::count(), static_cast<uint8_t>(10),
                  F("steady picker frames read no names"));
    test.expectEq(FxReadCounter::renderExact(0), true,
                  F("picker render reads no metadata"));

    FxReadCounter::resetFrame();
    for (uint8_t step = 0; step < 4; ++step)
        LureZone::pickerInput(LureZone::PICKER_BTN_DOWN);
    test.expectEq(LureZone::pickerWindow(), static_cast<uint8_t>(0),
                  F("in-window cursor moves keep the window"));
    test.expectEq(FxReadCounter::count(), static_cast<uint8_t>(0),
                  F("in-window picker moves read no names"));
    test.expectEq(FxReadCounter::markUpdate(), true,
                  F("in-window picker move is steady"));

    FxReadCounter::resetFrame();
    LureZone::pickerInput(LureZone::PICKER_BTN_DOWN);
    test.expectEq(LureZone::pickerWindow(), static_cast<uint8_t>(1),
                  F("the fifth cursor step scrolls the window"));
    test.expectEq(FxReadCounter::count(), static_cast<uint8_t>(10),
                  F("picker scroll resolves one window"));
    test.expectEq(FxReadCounter::markUpdate(), true,
                  F("picker scroll is a transition"));
    FxReadCounter::resetFrame();
    for (uint8_t step = 0; step < 4; ++step)
        LureZone::pickerInput(LureZone::PICKER_BTN_UP);
    test.expectEq(FxReadCounter::count(), static_cast<uint8_t>(0),
                  F("scroll-back cursor moves keep the cache"));
    LureZone::pickerInput(LureZone::PICKER_BTN_UP);
    test.expectEq(LureZone::pickerWindow(), static_cast<uint8_t>(0),
                  F("scroll-back reaches the first window"));
    test.expectEq(FxReadCounter::count(), static_cast<uint8_t>(10),
                  F("scroll back resolves one window"));
    test.expectEq(FxReadCounter::markUpdate(), true,
                  F("picker scroll back is a transition"));

    FxReadCounter::resetFrame();
    LureZone::pickerInput(LureZone::PICKER_BTN_B);
    test.expectEq(LureZone::pickerOpen(), false, F("B closes the picker"));
    test.expectEq(FxReadCounter::count(), static_cast<uint8_t>(0),
                  F("picker close reads nothing"));
    test.expectEq(FxReadCounter::markUpdate(), true, F("picker close stays steady"));

    // Deposit spends one lure and rebuilds the shifted window.
    FxReadCounter::resetFrame();
    LureZone::openPicker();
    FxReadCounter::resetFrame();
    LureZone::pickerInput(LureZone::PICKER_BTN_A);
    test.expectEq(LureZone::pickerFeedback(), true, F("deposit reports feedback"));
    test.expectEq(LureZone::active(), true, F("deposit arms the zone"));
    test.expectEq(LureZone::pickerCount(), static_cast<uint8_t>(5),
                  F("deposit spends exactly one lure"));
    test.expectEq(FxReadCounter::count(), static_cast<uint8_t>(10),
                  F("post-deposit window resolves five rows"));
    test.expectEq(FxReadCounter::markUpdate(), true,
                  F("deposit is a transition"));
    LureZone::drawOverlay();
    test.expectEq(FxReadCounter::count(), static_cast<uint8_t>(10),
                  F("post-deposit draw reads nothing"));
    LureZone::pickerInput(LureZone::PICKER_BTN_B);
    LureZone::clear();

    // Empty bag: open, A refusal and draw are all zero-read steady states.
    item::inventoryClear(LureZone::inventory());
    FxReadCounter::resetFrame();
    LureZone::openPicker();
    test.expectEq(LureZone::pickerCount(), static_cast<uint8_t>(0),
                  F("empty picker lists nothing"));
    test.expectEq(FxReadCounter::count(), static_cast<uint8_t>(0),
                  F("empty picker open reads nothing"));
    test.expectEq(FxReadCounter::markUpdate(), true,
                  F("empty picker open is steady"));
    LureZone::pickerInput(LureZone::PICKER_BTN_A);
    test.expectEq(LureZone::pickerFeedback(), false,
                  F("empty picker refuses the deposit"));
    LureZone::drawOverlay();
    test.expectEq(FxReadCounter::count(), static_cast<uint8_t>(0),
                  F("empty picker refusal and draw read nothing"));
    LureZone::pickerInput(LureZone::PICKER_BTN_B);

    // The last lure: depositing empties the list without a name lookup.
    item::inventoryAdd(LureZone::inventory(), item::ItemKind::Lure,
                       item::lureId(1, 0), 1);
    FxReadCounter::resetFrame();
    LureZone::openPicker();
    test.expectEq(FxReadCounter::count(), static_cast<uint8_t>(2),
                  F("single-lure open resolves one row"));
    FxReadCounter::resetFrame();
    LureZone::pickerInput(LureZone::PICKER_BTN_A);
    test.expectEq(LureZone::pickerCount(), static_cast<uint8_t>(0),
                  F("depositing the last lure empties the picker"));
    test.expectEq(FxReadCounter::count(), static_cast<uint8_t>(0),
                  F("emptied picker reads nothing"));
    test.expectEq(FxReadCounter::markUpdate(), true,
                  F("last-lure deposit is a transition"));
    LureZone::pickerInput(LureZone::PICKER_BTN_A);
    test.expectEq(LureZone::pickerCount(), static_cast<uint8_t>(0),
                  F("an emptied picker refuses A"));
    test.expectEq(FxReadCounter::count(), static_cast<uint8_t>(0),
                  F("emptied refusal reads nothing"));
    LureZone::pickerInput(LureZone::PICKER_BTN_B);
    LureZone::clear();
    item::inventoryClear(LureZone::inventory());
}

// --- World BAG/COLLECTION root --------------------------------------------
// Root navigation is RAM-only. The bag page uses the shared four-row cache;
// the collection list resolves one name per occupied visible row on open and on
// a window scroll, and the box-to-party swap installs one species read.
void menuRunWorld(FxTest &test) {
    item::Inventory &bag = LureZone::inventory();
    item::inventoryClear(bag);
    closeWorldMenu();

    // Empty bag through the root: every step and frame reads nothing.
    FxReadCounter::resetFrame();
    test.expectEq(collectionMenuWorldInput(INVENTORY_MENU_B, true, bag), true,
                  F("fresh B opens the root"));
    test.expectEq(FxReadCounter::count(), static_cast<uint8_t>(0),
                  F("root open reads nothing"));
    collectionMenuWorldInput(INVENTORY_MENU_A, false, bag);
    test.expectEq(static_cast<uint8_t>(collectionMenuWorldPage()),
                  static_cast<uint8_t>(CollectionPage::Bag),
                  F("A opens BAG"));
    test.expectEq(FxReadCounter::count(), static_cast<uint8_t>(0),
                  F("empty bag open reads nothing"));
    test.expectEq(FxReadCounter::markUpdate(), true,
                  F("empty bag open is steady"));
    for (uint8_t frame = 0; frame < 3; ++frame) {
        collectionMenuWorldInput(0, false, bag);
        collectionMenuWorldDraw(bag);
    }
    test.expectEq(FxReadCounter::count(), static_cast<uint8_t>(0),
                  F("empty bag frames read nothing"));
    collectionMenuWorldInput(INVENTORY_MENU_A, false, bag); // never takes an item
    test.expectEq(item::inventoryNonZeroCount(bag), static_cast<uint8_t>(0),
                  F("BAG A never takes the empty bag"));
    collectionMenuWorldInput(INVENTORY_MENU_B, false, bag);
    collectionMenuWorldInput(INVENTORY_MENU_B, false, bag);
    test.expectEq(collectionMenuWorldOpen(), false, F("root B closes the menu"));

    // Mixed bag: four lures (two parts) then one consumable (one part).
    for (uint8_t id = 0; id < 4; ++id)
        item::inventoryAdd(bag, item::ItemKind::Lure, id, 1);
    item::inventoryAdd(bag, item::ItemKind::Consumable, 4, 1);
    FxReadCounter::resetFrame();
    collectionMenuWorldInput(INVENTORY_MENU_B, true, bag);
    collectionMenuWorldInput(INVENTORY_MENU_A, false, bag);
    test.expectEq(FxReadCounter::count(), static_cast<uint8_t>(8),
                  F("bag open resolves four lure rows"));
    test.expectEq(FxReadCounter::markUpdate(), true,
                  F("bag open is a transition"));
    FxReadCounter::resetFrame();
    for (uint8_t step = 0; step < 3; ++step)
        collectionMenuWorldInput(INVENTORY_MENU_DOWN, false, bag);
    test.expectEq(FxReadCounter::count(), static_cast<uint8_t>(0),
                  F("in-window bag moves read nothing"));
    test.expectEq(FxReadCounter::markUpdate(), true,
                  F("in-window bag move is steady"));
    FxReadCounter::resetFrame();
    collectionMenuWorldInput(INVENTORY_MENU_DOWN, false, bag);
    test.expectEq(FxReadCounter::count(), static_cast<uint8_t>(7),
                  F("bag scroll resolves the shifted window"));
    test.expectEq(FxReadCounter::markUpdate(), true,
                  F("bag scroll is a transition"));
    for (uint8_t frame = 0; frame < 2; ++frame) collectionMenuWorldDraw(bag);
    test.expectEq(FxReadCounter::count(), static_cast<uint8_t>(7),
                  F("bag draw reads only the cache"));
    test.expectEq(FxReadCounter::renderExact(0), true,
                  F("bag render reads no metadata"));
    collectionMenuWorldInput(INVENTORY_MENU_B, false, bag);
    item::inventoryClear(bag);

    // Empty collection list, then a populated one: open/scroll/page/back.
    eraseStore();
    closeWorldMenu();
    collectionMenuWorldInput(INVENTORY_MENU_B, true, bag);
    collectionMenuWorldInput(INVENTORY_MENU_DOWN, false, bag);
    FxReadCounter::resetFrame();
    collectionMenuWorldInput(INVENTORY_MENU_A, false, bag);
    test.expectEq(static_cast<uint8_t>(collectionMenuWorldPage()),
                  static_cast<uint8_t>(CollectionPage::List),
                  F("A opens the empty collection"));
    test.expectEq(FxReadCounter::count(), static_cast<uint8_t>(0),
                  F("empty collection open reads nothing"));
    test.expectEq(FxReadCounter::markUpdate(), true,
                  F("empty collection open is steady"));
    collectionMenuWorldInput(INVENTORY_MENU_A, false, bag); // no row to confirm
    test.expectEq(FxReadCounter::count(), static_cast<uint8_t>(0),
                  F("empty collection A reads nothing"));
    collectionMenuWorldInput(INVENTORY_MENU_B, false, bag); // back to root
    collectionMenuWorldInput(INVENTORY_MENU_B, false, bag); // close

    for (uint8_t i = 0; i < 2; ++i) {
        uint8_t slot = 255;
        storeAdd(storedCreature(static_cast<uint8_t>(10 + i),
                                static_cast<uint16_t>(10 + i)), slot);
    }
    closeWorldMenu();
    collectionMenuWorldInput(INVENTORY_MENU_B, true, bag);
    collectionMenuWorldInput(INVENTORY_MENU_DOWN, false, bag);
    FxReadCounter::resetFrame();
    collectionMenuWorldInput(INVENTORY_MENU_A, false, bag);
    test.expectEq(FxReadCounter::count(), static_cast<uint8_t>(2),
                  F("two live rows resolve two names"));
    test.expectEq(FxReadCounter::markUpdate(), true,
                  F("collection open is a transition"));
    FxReadCounter::resetFrame();
    collectionMenuWorldInput(INVENTORY_MENU_DOWN, false, bag);
    test.expectEq(FxReadCounter::count(), static_cast<uint8_t>(0),
                  F("in-window collection move reads nothing"));
    collectionMenuWorldInput(INVENTORY_MENU_A, false, bag); // party page
    test.expectEq(static_cast<uint8_t>(collectionMenuWorldPage()),
                  static_cast<uint8_t>(CollectionPage::Party),
                  F("A opens the party-slot page"));
    collectionMenuWorldInput(INVENTORY_MENU_DOWN, false, bag);
    collectionMenuWorldInput(INVENTORY_MENU_A, false, bag); // confirm slot 2
    test.expectEq(collectionMenuWorldChoice().partySlot, static_cast<uint8_t>(2),
                  F("confirmation carries party slot 2"));
    test.expectEq(FxReadCounter::count(), static_cast<uint8_t>(0),
                  F("party page navigation reads nothing"));
    test.expectEq(FxReadCounter::markUpdate(), true,
                  F("party page navigation is steady"));
    collectionMenuWorldInput(INVENTORY_MENU_B, false, bag); // list
    collectionMenuWorldInput(INVENTORY_MENU_B, false, bag); // root
    collectionMenuWorldInput(INVENTORY_MENU_B, false, bag); // closed
    test.expectEq(collectionMenuWorldOpen(), false, F("root B closes the menu"));

    // Eight instances: the window scrolls one four-row page at a time.
    eraseStore();
    for (uint8_t i = 0; i < 8; ++i) {
        uint8_t slot = 255;
        storeAdd(storedCreature(static_cast<uint8_t>(10 + i),
                                static_cast<uint16_t>(20 + i)), slot);
    }
    closeWorldMenu();
    collectionMenuWorldInput(INVENTORY_MENU_B, true, bag);
    collectionMenuWorldInput(INVENTORY_MENU_DOWN, false, bag);
    FxReadCounter::resetFrame();
    collectionMenuWorldInput(INVENTORY_MENU_A, false, bag);
    test.expectEq(FxReadCounter::count(), static_cast<uint8_t>(4),
                  F("eight-row collection open resolves one window"));
    test.expectEq(FxReadCounter::markUpdate(), true,
                  F("collection open stays in budget"));
    FxReadCounter::resetFrame();
    for (uint8_t step = 0; step < 3; ++step)
        collectionMenuWorldInput(INVENTORY_MENU_DOWN, false, bag);
    test.expectEq(FxReadCounter::count(), static_cast<uint8_t>(0),
                  F("three in-window steps read nothing"));
    collectionMenuWorldInput(INVENTORY_MENU_DOWN, false, bag);
    test.expectEq(FxReadCounter::count(), static_cast<uint8_t>(4),
                  F("the fourth step scrolls one window"));
    test.expectEq(FxReadCounter::markUpdate(), true,
                  F("collection scroll is a transition"));
    collectionMenuWorldDraw(bag);
    test.expectEq(FxReadCounter::count(), static_cast<uint8_t>(4),
                  F("collection draw reads only the cache"));
    test.expectEq(FxReadCounter::renderExact(0), true,
                  F("collection render reads no metadata"));
    collectionMenuWorldInput(INVENTORY_MENU_B, false, bag);
    collectionMenuWorldInput(INVENTORY_MENU_B, false, bag);

    // Box-to-party replacement installs exactly one species read; the
    // already-assigned refusal reads nothing and mutates nothing.
    eraseStore();
    player = Player();
    uint8_t outgoing = 255;
    uint8_t incoming = 255;
    uint8_t fullHP = 255;
    storeAdd(storedCreature(3, 126, 5), outgoing); // 4 current HP
    storeAdd(storedCreature(6, 200, 3), incoming); // 2 current HP
    storeAdd(storedCreature(9, 30, 0), fullHP);    // legacy full HP
    storeAssign(0, outgoing, 4);
    player.creatureHPs[0] = 4;
    closeWorldMenu();
    collectionMenuWorldInput(INVENTORY_MENU_B, true, bag);
    collectionMenuWorldInput(INVENTORY_MENU_DOWN, false, bag);
    collectionMenuWorldInput(INVENTORY_MENU_A, false, bag); // list
    FxReadCounter::resetFrame();
    collectionMenuWorldInput(INVENTORY_MENU_A, false, bag); // already in party
    collectionMenuWorldInput(INVENTORY_MENU_A, false, bag); // confirm slot 1
    test.expectEq(collectionMenuWorldApplyChoice(player), true,
                  F("already-assigned choice is consumed"));
    test.expectEq(static_cast<uint8_t>(collectionMenuWorldFeedback()),
                  static_cast<uint8_t>(CollectionFeedback::InParty),
                  F("already-assigned refusal is reported"));
    test.expectEq(FxReadCounter::count(), static_cast<uint8_t>(0),
                  F("already-assigned refusal reads nothing"));
    test.expectEq(FxReadCounter::markUpdate(), true,
                  F("already-assigned refusal stays steady"));

    collectionMenuWorldInput(INVENTORY_MENU_DOWN, false, bag); // row 1
    collectionMenuWorldInput(INVENTORY_MENU_A, false, bag);    // party page
    collectionMenuWorldInput(INVENTORY_MENU_A, false, bag);    // confirm slot 1
    FxReadCounter::resetFrame();
    test.expectEq(collectionMenuWorldApplyChoice(player), true,
                  F("box-to-party swap is consumed"));
    test.expectEq(FxReadCounter::count(), static_cast<uint8_t>(3),
                  F("swap resolves its journal and member reads"));
    test.expectEq(FxReadCounter::markUpdate(), true,
                  F("swap transition is declared"));
    test.expectEq(player.party[0].id, static_cast<uint8_t>(6),
                  F("the incoming instance is installed"));
    test.expectEq(player.creatureHPs[0], static_cast<uint8_t>(2),
                  F("the incoming instance keeps its stored HP"));

    // A legacy full-HP instance adds the one max-HP lookup the encoded-HP
    // record omits, so the boundary count is data-dependent by design.
    collectionMenuWorldInput(INVENTORY_MENU_DOWN, false, bag); // row 2
    collectionMenuWorldInput(INVENTORY_MENU_A, false, bag);    // party page
    collectionMenuWorldInput(INVENTORY_MENU_A, false, bag);    // confirm slot 1
    FxReadCounter::resetFrame();
    test.expectEq(collectionMenuWorldApplyChoice(player), true,
                  F("full-HP swap is consumed"));
    test.expectEq(FxReadCounter::count(), static_cast<uint8_t>(4),
                  F("full-HP swap resolves one extra max-HP read"));
    test.expectEq(FxReadCounter::markUpdate(), true,
                  F("full-HP swap transition is declared"));
    test.expectEq(player.party[0].id, static_cast<uint8_t>(9),
                  F("the full-HP instance is installed"));
    test.expectEq(player.creatureHPs[0], storePartyHP(0),
                  F("the full-HP instance keeps its maximum"));
    test.expectEq(player.creatureHPs[0] != 0, true,
                  F("the full-HP instance is live"));
    closeWorldMenu();
    item::inventoryClear(bag);
}

inline void test_menurun(FxTest &test) {
    menuRunDialogs(test);
    menuRunPicker(test);
    menuRunWorld(test);
}

} // namespace menurun_test_detail
