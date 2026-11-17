#pragma once

#include "test.hpp"
#include <Arduboy2.h>
#include "src/FXDataFake.hpp"
#include "src/WorldInteractionFake.hpp"
#include "../src/engine/menu/InventoryMenu.hpp"
#include "../src/engine/menu/CollectionMenu.hpp"
#include "../src/engine/world/LureZone.hpp"
#include "../src/engine/world/World.hpp"
#include "../src/globals.hpp"
#include "../src/item/Inventory.hpp"
#include "../src/item/ItemIds.hpp"
#include "../src/lib/FxReadCounter.hpp"

// Defined in tst/main.cpp beside the other world input fakes.
namespace worldMovementFake {
void reset();
void requestButtons(uint8_t buttons);
uint16_t inputCalls();
}

void InventoryMenuTest(TestSuite &suite) {
    Test test(__func__);
    item::Inventory inv;
    item::inventoryClear(inv);
    InventoryMenu menu = {};

    // Empty bag: no rows, no cached names and no metadata lookup.
    FxReadCounter::resetFrame();
    inventoryMenuOpen(menu, inv);
    test.assert(menu.view.itemCount, static_cast<uint8_t>(0), "empty bag has no rows");
    test.assert(menu.rowParts[0], static_cast<uint8_t>(0), "empty row cache is clear");
    test.assert(FxReadCounter::count(), static_cast<uint8_t>(0),
                "opening an empty bag resolves no names");
    test.assert(inventoryMenuSelect(menu).kind, MenuIntentKind::None,
                "empty bag selects nothing");
    const uint8_t emptyReads = FxReadCounter::count();
    drawInventoryMenu(menu, inv);
    test.assert(FxReadCounter::count(), emptyReads,
                "drawing the empty page resolves no names");

    // One consumable: single row, one name part.
    item::inventoryAdd(inv, item::ItemKind::Consumable, 2, 1);
    inventoryMenuOpen(menu, inv);
    test.assert(menu.view.itemCount, static_cast<uint8_t>(1), "single item opens one row");
    test.assert(menu.rowKind[0], static_cast<uint8_t>(item::ItemKind::Consumable),
                "single row caches item kind");
    test.assert(menu.rowId[0], static_cast<uint8_t>(2), "single row caches item id");
    test.assert(menu.rowParts[0], static_cast<uint8_t>(1), "consumable name has one part");
    test.assert(menu.rowName[0][0] != 0, true, "consumable name part is cached");
    const MenuIntent single = inventoryMenuSelect(menu);
    test.assert(single.kind, MenuIntentKind::SelectItem, "selection returns item intent");
    test.assert(single.index, static_cast<uint8_t>(0), "selection is the visible row");
    test.assert(menu.rowKind[single.index], static_cast<uint8_t>(item::ItemKind::Consumable),
                "visible row carries the semantic kind");
    test.assert(menu.rowId[single.index], static_cast<uint8_t>(2),
                "visible row carries the semantic id");
    const uint8_t singleReads = FxReadCounter::count();
    drawInventoryMenu(menu, inv);
    test.assert(FxReadCounter::count(), singleReads,
                "draw uses cached consumable address without table reads");

    // Five mixed rows; a lure shows both name parts.
    item::inventoryClear(inv);
    for (uint8_t id = 0; id < 4; ++id)
        item::inventoryAdd(inv, item::ItemKind::Lure, id, 1);
    item::inventoryAdd(inv, item::ItemKind::Consumable, 3, 1);
    FxReadCounter::resetFrame();
    inventoryMenuOpen(menu, inv);
    test.assert(menu.view.itemCount, static_cast<uint8_t>(5), "five nonzero items are listed");
    test.assert(menu.view.windowStart, static_cast<uint8_t>(0), "initial window starts at zero");
    test.assert(menu.rowId[0], static_cast<uint8_t>(0), "first row follows inventory order");
    test.assert(menu.rowKind[0], static_cast<uint8_t>(item::ItemKind::Lure),
                "lures precede consumables");
    test.assert(menu.rowParts[0], static_cast<uint8_t>(2), "lure name has two parts");
    test.assert(menu.rowName[0][0] != 0, true, "lure tier name is cached");
    test.assert(menu.rowName[0][1] != 0, true, "lure type name is cached");
    test.assert(menu.rowId[3], static_cast<uint8_t>(3), "fourth row is the fourth lure");
    test.assert(menu.rowKind[3], static_cast<uint8_t>(item::ItemKind::Lure),
                "the consumable sits beyond the first window");
    test.assert(FxReadCounter::count(), static_cast<uint8_t>(8),
                "open resolves four lure rows once");

    // In-window cursor motion must not reload names; scrolling must.
    inventoryMenuMove(menu, inv, 1);
    test.assert(menu.view.cursor, static_cast<uint8_t>(1), "move advances selection");
    test.assert(menu.view.windowStart, static_cast<uint8_t>(0), "cursor stays in the window");
    test.assert(FxReadCounter::count(), static_cast<uint8_t>(8),
                "in-window move performs no name lookup");
    inventoryMenuMove(menu, inv, 100);
    test.assert(menu.view.cursor, static_cast<uint8_t>(4), "move clamps at end");
    test.assert(menu.view.windowStart, static_cast<uint8_t>(1), "window follows end cursor");
    test.assert(menu.rowId[0], static_cast<uint8_t>(1), "scroll refreshes first cached row");
    test.assert(menu.rowKind[3], static_cast<uint8_t>(item::ItemKind::Consumable),
                "scroll refreshes final cached row kind");
    test.assert(menu.rowParts[3], static_cast<uint8_t>(1), "consumable row has one part");
    test.assert(FxReadCounter::count(), static_cast<uint8_t>(15),
                "scroll resolves the shifted window once");
    const MenuIntent scrolled = inventoryMenuSelect(menu);
    test.assert(scrolled.kind, MenuIntentKind::SelectItem, "scrolled selection is item intent");
    test.assert(scrolled.index, static_cast<uint8_t>(3), "selection is the visible row");
    test.assert(menu.rowId[scrolled.index], static_cast<uint8_t>(3),
                "row 3 now carries the consumable id");
    test.assert(menu.rowKind[scrolled.index], static_cast<uint8_t>(item::ItemKind::Consumable),
                "row 3 no longer aliases the off-window lure id");
    const uint8_t readsAtEnd = FxReadCounter::count();
    drawInventoryMenu(menu, inv);
    test.assert(FxReadCounter::count(), readsAtEnd, "draw after scroll performs zero lookups");
    inventoryMenuMove(menu, inv, -100);
    test.assert(menu.view.cursor, static_cast<uint8_t>(0), "scroll returns to beginning");
    test.assert(menu.view.windowStart, static_cast<uint8_t>(0), "window returns to beginning");
    test.assert(menu.rowId[0], static_cast<uint8_t>(0), "scroll back refreshes cached row");

    // Full nonzero bag: 24 lures then 8 consumables, non-wrapping ends.
    item::inventoryClear(inv);
    for (uint8_t id = 0; id < item::LURE_COUNT; ++id)
        item::inventoryAdd(inv, item::ItemKind::Lure, id, 1);
    for (uint8_t id = 0; id < item::CONSUMABLE_COUNT; ++id)
        item::inventoryAdd(inv, item::ItemKind::Consumable, id, 1);
    FxReadCounter::resetFrame();
    inventoryMenuOpen(menu, inv);
    test.assert(menu.view.itemCount, static_cast<uint8_t>(32), "full bag lists 32 rows");
    inventoryMenuMove(menu, inv, -100);
    test.assert(menu.view.cursor, static_cast<uint8_t>(0), "up at the first row does not wrap");
    inventoryMenuMove(menu, inv, 100);
    test.assert(menu.view.cursor, static_cast<uint8_t>(31), "down clamps at the last row");
    test.assert(menu.view.windowStart, static_cast<uint8_t>(28), "end window holds four rows");
    test.assert(menu.rowId[0], static_cast<uint8_t>(4), "end window starts at consumable 4");
    test.assert(menu.rowId[3], static_cast<uint8_t>(7), "end window ends at consumable 7");
    test.assert(menu.rowKind[0], static_cast<uint8_t>(item::ItemKind::Consumable),
                "end window shows consumables");
    const MenuIntent last = inventoryMenuSelect(menu);
    test.assert(last.index, static_cast<uint8_t>(3), "last selection is the visible row");
    test.assert(menu.rowId[last.index], static_cast<uint8_t>(7), "last row id is consumable 7");
    drawInventoryMenu(menu, inv);
    inventoryMenuMove(menu, inv, 1);
    test.assert(menu.view.cursor, static_cast<uint8_t>(31), "down at the last row does not wrap");

    // World overlay through the BAG/COLLECTION root menu: fresh B opens the
    // root, A opens the bag, B closes it back to the root, and world motion
    // stays frozen for every frame a page is up.
    LureZone::dismissIntro();
    LureZone::closePicker();
    dialogMenu.clear();
    item::inventoryClear(LureZone::inventory());
    item::inventoryAdd(LureZone::inventory(), item::ItemKind::Lure, item::lureId(1, 0), 2);
    item::inventoryAdd(LureZone::inventory(), item::ItemKind::Consumable, 4, 3);
    gameState.playerLocation = static_cast<uint16_t>((64u << 8) | 7u);
    WorldEngine::init(worldState());
    worldMovementFake::reset();
    test.assert(inventoryMenuWorldInput(INVENTORY_MENU_B, false, LureZone::inventory()), false,
                "B cannot open the bag while the world is busy");
    test.assert(inventoryMenuWorldOpen(), false, "busy world leaves the bag closed");
    worldInteractionFake::pickerButtons = INVENTORY_MENU_B;
    WorldEngine::runMap(worldState());
    test.assert(collectionMenuWorldOpen(), true, "fresh B opens the world root menu");
    test.assert(inventoryMenuWorldOpen(), false, "the root menu does not open the bag directly");
    worldInteractionFake::pickerButtons = INVENTORY_MENU_A;
    WorldEngine::runMap(worldState());
    test.assert(inventoryMenuWorldOpen(), true, "A on BAG opens the bag page");
    test.assert(inventoryMenuWorldInput(0, false, LureZone::inventory()), true,
                "open page owns the world frame even when not idle");

    const uint16_t frozenLocation = gameState.playerLocation;
    worldInteractionFake::pickerButtons = 0;
    worldMovementFake::requestButtons(LEFT_BUTTON);
    WorldEngine::runMap(worldState());
    test.assert(worldMovementFake::inputCalls(), static_cast<uint16_t>(0),
                "open page never samples world movement");
    test.assert(gameState.playerLocation, frozenLocation,
                "open page cannot advance world movement");
    test.assert(worldState().motion.step, static_cast<uint8_t>(0),
                "open page leaves the step cursor parked");

    const uint8_t beforeCount = item::inventoryCount(LureZone::inventory(),
                                                     item::ItemKind::Lure,
                                                     item::lureId(1, 0));
    WorldEngine::runMap(worldState()); // A edge is not set; page still owns frame
    worldInteractionFake::pickerButtons = INVENTORY_MENU_A;
    WorldEngine::runMap(worldState());
    test.assert(item::inventoryCount(LureZone::inventory(), item::ItemKind::Lure,
                                     item::lureId(1, 0)),
                beforeCount, "A selection does not consume an item");
    test.assert(inventoryMenuWorldOpen(), true, "A selection keeps the page open");

    worldInteractionFake::pickerButtons = INVENTORY_MENU_B;
    WorldEngine::runMap(worldState());
    test.assert(inventoryMenuWorldOpen(), false, "fresh B closes the bag page");
    test.assert(collectionMenuWorldOpen(), true, "B from the bag returns to the root menu");
    worldInteractionFake::pickerButtons = 0;
    worldMovementFake::reset();
    WorldEngine::runMap(worldState());
    test.assert(gameState.playerLocation, frozenLocation,
                "the root menu still freezes the world");
    worldInteractionFake::pickerButtons = INVENTORY_MENU_B;
    WorldEngine::runMap(worldState());
    test.assert(collectionMenuWorldOpen(), false, "fresh B closes the root menu");
    worldInteractionFake::pickerButtons = 0;
    worldMovementFake::reset();
    WorldEngine::runMap(worldState());
    test.assert(gameState.playerLocation, frozenLocation, "world resumes without stepping");
    worldInteractionFake::pickerButtons = INVENTORY_MENU_B;
    WorldEngine::runMap(worldState());
    test.assert(collectionMenuWorldOpen(), true, "a later fresh B reopens the root menu");
    worldInteractionFake::pickerButtons = INVENTORY_MENU_A;
    WorldEngine::runMap(worldState());
    test.assert(inventoryMenuWorldOpen(), true, "A on BAG reopens the bag page");
    worldInteractionFake::pickerButtons = 0;
    collectionMenuWorldInput(INVENTORY_MENU_B, true, LureZone::inventory());
    collectionMenuWorldInput(INVENTORY_MENU_B, true, LureZone::inventory());
    test.assert(inventoryMenuWorldOpen(), false, "final close leaves the world page clear");
    test.assert(collectionMenuWorldOpen(), false, "final close leaves the root menu clear");
    item::inventoryClear(LureZone::inventory());

    test.assert(fxDataFake::readCount > 0, true, "open and scroll resolve names through FX fake");
    suite.addTest(test);
}

void InventoryMenuSuite(TestRunner &runner) {
    TestSuite suite("Inventory Menu");
    InventoryMenuTest(suite);
    runner.addTestSuite(suite);
}
