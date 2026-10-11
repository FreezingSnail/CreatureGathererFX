#pragma once

#include "test.hpp"
#include "src/FXDataFake.hpp"
#include "../src/GameState.hpp"
#include "../src/engine/world/Chunk.hpp"
#include "../src/engine/world/LureZone.hpp"
#include "../src/item/Inventory.hpp"
#include "../src/item/ItemIds.hpp"

extern GameState gameState;

// Deposit/decay/expiry state machine for the overworld lure zone. The step
// hook integration (decay through onStep, the 25% roll gate, and chunk-change
// clearing) is covered in step_test.hpp; this suite pins the state machine.
void LureZoneStateTest(TestSuite &suite) {
    Test test(__func__);

    LureZone::clear();
    test.assert(LureZone::active(), false, "cleared zone is inert");
    test.assert(LureZone::tableOffset(), static_cast<uint8_t>(LureZone::INERT_OFFSET),
                "inert zone has no lure table offset");
    test.assert(LureZone::zoneChunk(), static_cast<uint16_t>(LureZone::NO_CHUNK),
                "inert zone owns no chunk");

    test.assert(LureZone::lureId(2, 1), static_cast<uint8_t>(6),
                "lure id packs type and tier");
    test.assert(LureZone::lureType(6), static_cast<uint8_t>(2),
                "lure type is the low two bits");
    test.assert(LureZone::lureTier(6), static_cast<uint8_t>(1),
                "lure tier is the high bits");

    gameState.playerLocation = 0x0209;
    const uint16_t chunk = Chunk::chunkOfLocation(gameState.playerLocation);
    LureZone::deposit(LureZone::lureId(1, 0));
    test.assert(LureZone::active(), true, "deposit activates the zone");
    test.assert(LureZone::tableOffset(), static_cast<uint8_t>(1),
                "active lure exposes its type");
    test.assert(LureZone::charge(), LureZone::lureChargeOf(0),
                "deposit sets the tier charge");
    test.assert(LureZone::zoneChunk(), chunk, "deposit records the current chunk");

    const uint8_t charge = LureZone::charge();
    for (uint8_t step = 0; step < charge; ++step) LureZone::onStep();
    test.assert(LureZone::charge(), static_cast<uint8_t>(0),
                "steps decay the charge to zero");
    test.assert(LureZone::active(), false, "expiry clears the lure id");
    test.assert(LureZone::tableOffset(), static_cast<uint8_t>(LureZone::INERT_OFFSET),
                "expired lure is inert");

    LureZone::deposit(LureZone::lureId(3, 2));
    test.assert(LureZone::charge(), LureZone::lureChargeOf(2),
                "tier two charge is longer");
    LureZone::clear();
    test.assert(LureZone::active(), false, "clear drops the lure");
    test.assert(LureZone::charge(), static_cast<uint8_t>(0), "clear zeroes the charge");

    LureZone::deposit(LureZone::lureId(0, 0));
    LureZone::deposit(LureZone::LURE_NONE);
    test.assert(LureZone::active(), false, "none deposit clears the zone");

    suite.addTest(test);
}

// Material picker state machine: lure-only filtering, cursor/window movement,
// consume-then-deposit, feedback, and the non-blocking expiry message.
void LurePickerTest(TestSuite &suite) {
    Test test(__func__);
    item::Inventory &inv = LureZone::inventory();
    item::inventoryClear(inv);
    LureZone::closePicker();
    LureZone::clear();
    gameState.playerLocation = 0x0209;

    LureZone::openPicker();
    test.assert(LureZone::pickerOpen(), true, "open sets the picker");
    test.assert(LureZone::pickerCount(), static_cast<uint8_t>(0),
                "empty inventory lists nothing");
    test.assert(LureZone::pickerDeposit(), false, "empty picker refuses a deposit");

    item::inventoryAdd(inv, item::ItemKind::Lure, item::lureId(1, 1), 2);
    item::inventoryAdd(inv, item::ItemKind::Lure, item::lureId(2, 2), 1);
    item::inventoryAdd(inv, item::ItemKind::Consumable, 3, 1);
    LureZone::openPicker();
    test.assert(LureZone::pickerCount(), static_cast<uint8_t>(2),
                "list filters out consumables");
    uint8_t id;
    uint8_t parts;
    uint24_t name0;
    uint24_t name1;
    test.assert(LureZone::pickerRow(0, id, parts, name0, name1), true,
                "first row resolves a lure name");
    test.assert(id, item::lureId(1, 1), "rows follow ascending lure id");
    test.assert(parts, static_cast<uint8_t>(2), "lure name has tier then type");
    test.assert(name0 != name1, true, "tier and type addresses differ");

    test.assert(LureZone::pickerDeposit(), true, "A consumes the cursor lure");
    test.assert(item::inventoryCount(inv, item::ItemKind::Lure, item::lureId(1, 1)),
                static_cast<uint8_t>(1), "deposit takes exactly one unit");
    test.assert(LureZone::active(), true, "deposit activates the zone");
    test.assert(LureZone::id(), LureZone::lureId(1, 1),
                "zone keeps the deposited type and tier");
    test.assert(LureZone::pickerFeedback(), true, "deposit sets feedback");
    test.assert(LureZone::pickerCount(), static_cast<uint8_t>(2),
                "consumed lure remains while a unit is left");

    LureZone::pickerInput(LureZone::PICKER_BTN_DOWN);
    test.assert(LureZone::pickerCursor(), static_cast<uint8_t>(1),
                "down advances the cursor");
    test.assert(LureZone::pickerFeedback(), false, "movement clears feedback");
    LureZone::pickerInput(LureZone::PICKER_BTN_UP);
    test.assert(LureZone::pickerCursor(), static_cast<uint8_t>(0),
                "up returns the cursor");
    LureZone::pickerInput(LureZone::PICKER_BTN_B);
    test.assert(LureZone::pickerOpen(), false, "B closes the picker");

    // The picker also exposes up to five rows of a larger list.
    item::inventoryClear(inv);
    for (uint8_t i = 0; i < item::LURE_COUNT; ++i)
        item::inventoryAdd(inv, item::ItemKind::Lure, i, 1);
    LureZone::openPicker();
    test.assert(LureZone::pickerCount(), item::LURE_COUNT,
                "full lure inventory lists every id");
    for (uint8_t i = 0; i < item::LURE_COUNT; ++i)
        LureZone::pickerInput(LureZone::PICKER_BTN_DOWN);
    test.assert(LureZone::pickerCursor(), static_cast<uint8_t>(item::LURE_COUNT - 1),
                "cursor stops at the list end");
    test.assert(LureZone::pickerWindow(),
                static_cast<uint8_t>(item::LURE_COUNT - LureZone::PICKER_ROWS),
                "window follows the end cursor");
    test.assert(LureZone::pickerRow(0, id, parts, name0, name1), true,
                "scrolled window resolves its first row");
    test.assert(id, static_cast<uint8_t>(item::LURE_COUNT - LureZone::PICKER_ROWS),
                "scrolled window starts at the expected id");
    LureZone::pickerInput(LureZone::PICKER_BTN_B);
    item::inventoryClear(inv);

    // Expiry toast: the message window decays and expiry announces itself.
    LureZone::showMessage(0x12345, 10, 1);
    test.assert(LureZone::messageActive(), true, "message is active after show");
    LureZone::onStep();
    test.assert(LureZone::messageActive(), false, "message expires after its steps");

    LureZone::clear();
    LureZone::deposit(LureZone::lureId(0, 0));
    const uint8_t charge = LureZone::charge();
    for (uint8_t step = 0; step < charge; ++step) LureZone::onStep();
    test.assert(LureZone::active(), false, "lure expires through the step hook");
    test.assert(LureZone::messageActive(), true, "expiry raises a message");

    suite.addTest(test);
}

void LureZoneSuite(TestRunner &runner) {
    TestSuite suite("Lure zone state");
    LureZoneStateTest(suite);
    LurePickerTest(suite);
    runner.addTestSuite(suite);
}
