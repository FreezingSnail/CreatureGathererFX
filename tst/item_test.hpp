#pragma once

#include "../src/item/ItemIds.hpp"
#include "../src/item/Inventory.hpp"
#include "../src/item/KeyItems.hpp"
#include "../src/item/ConsumableDef.hpp"
#include "../src/item/ItemNames.hpp"
#include "src/FXDataFake.hpp"
#include "../src/fxdata.h"
#include "../src/flags/flag_bit_array.hpp"
#include "test.hpp"

void ItemSuite(TestRunner &runner) {
    TestSuite suite = TestSuite("Item IDs");
    Test test = Test(__func__);
    for (uint8_t id = 0; id < item::LURE_COUNT; ++id) {
        test.assert(item::lureId(item::lureTypeOf(id), item::lureTierOf(id)), id,
                    "lure id round trips through type and tier");
    }

    test.assert(item::lureTypeOf(23), static_cast<uint8_t>(7), "last lure type is 7");
    test.assert(item::lureTierOf(23), static_cast<uint8_t>(2), "last lure tier is 2");

    test.assert(item::lureChargeOf(0), static_cast<uint8_t>(16), "tier 0 charge");
    test.assert(item::lureChargeOf(1), static_cast<uint8_t>(32), "tier 1 charge");
    test.assert(item::lureChargeOf(2), static_cast<uint8_t>(64), "tier 2 charge");
    test.assert(item::lureRateOf(0), static_cast<uint8_t>(1), "tier 0 rate");
    test.assert(item::lureRateOf(1), static_cast<uint8_t>(2), "tier 1 rate");
    test.assert(item::lureRateOf(2), static_cast<uint8_t>(4), "tier 2 rate");

    fxDataFake::dataBase = consumable_table;
    fxDataFake::dataBytes[0] = static_cast<uint8_t>(item::ConsumableKind::Heal);
    fxDataFake::dataBytes[1] = 16;
    fxDataFake::dataBytes[14] = static_cast<uint8_t>(item::ConsumableKind::Charge);
    fxDataFake::dataBytes[15] = 32;

    fxDataFake::readCount = 0;
    item::ConsumableDef consumable = item::readConsumableDef(0);
    test.assert(consumable.kind, static_cast<uint8_t>(item::ConsumableKind::Heal),
                "consumable zero decodes its kind");
    test.assert(consumable.arg, static_cast<uint8_t>(16), "consumable zero decodes its arg");
    test.assert(fxDataFake::lastDataAddress, consumable_table,
                "consumable zero reads its table address");
    test.assert(fxDataFake::lastDataLength, static_cast<size_t>(2),
                "consumable read is two bytes");
    test.assert(fxDataFake::readCount, static_cast<uint32_t>(1),
                "consumable zero uses one FX read");

    fxDataFake::readCount = 0;
    consumable = item::readConsumableDef(7);
    test.assert(consumable.kind, static_cast<uint8_t>(item::ConsumableKind::Charge),
                "consumable seven decodes its kind");
    test.assert(consumable.arg, static_cast<uint8_t>(32), "consumable seven decodes its arg");
    test.assert(fxDataFake::lastDataAddress,
                consumable_table + static_cast<uint16_t>(7) * 2,
                "consumable id uses a two-byte address stride");
    test.assert(fxDataFake::lastDataLength, static_cast<size_t>(2),
                "last consumable read is two bytes");
    test.assert(fxDataFake::readCount, static_cast<uint32_t>(1),
                "consumable seven uses one FX read");

    fxDataFake::readCount = 0;
    consumable = item::readConsumableDef(8);
    test.assert(consumable.kind, static_cast<uint8_t>(item::ConsumableKind::None),
                "out-of-range consumable kind is None");
    test.assert(consumable.arg, static_cast<uint8_t>(0), "out-of-range consumable arg is zero");
    test.assert(fxDataFake::readCount, static_cast<uint32_t>(0),
                "out-of-range consumable performs no FX read");

    item::KeyItems keyItems;
    item::keyItemsClear(keyItems);
    test.assert(sizeof(keyItems), static_cast<size_t>(1), "key item unlocked storage is one byte");
    test.assert(keyItems.bits[0], static_cast<uint8_t>(0), "clear zeros key item bits");
    for (uint8_t keyId = 0; keyId < item::KEY_ITEM_COUNT; ++keyId) {
        item::keyItemUnlock(keyItems, keyId);
        test.assert(keyItems.bits[0], static_cast<uint8_t>(1U << keyId),
                    "unlock sets exactly its independent bit");
        test.assert(item::keyItemUnlocked(keyItems, keyId), true, "unlocked observes set bit");
        item::keyItemsClear(keyItems);
    }
    item::keyItemUnlock(keyItems, 3);
    item::keyItemUnlock(keyItems, 3);
    test.assert(keyItems.bits[0], static_cast<uint8_t>(1U << 3), "unlock is idempotent");
    test.assert(item::keyItemUnlocked(keyItems, 8), false, "unlocked rejects id 8");
    test.assert(item::keyItemUnlocked(keyItems, 255), false, "unlocked rejects id 255");
    const uint8_t keyBitsBeforeInvalid = keyItems.bits[0];
    item::keyItemUnlock(keyItems, 8);
    item::keyItemUnlock(keyItems, 255);
    test.assert(keyItems.bits[0], keyBitsBeforeInvalid, "unlock ignores invalid ids");
    FLAG_BIT_ARRAY[0] = 0;
    item::keyItemUnlock(keyItems, 0);
    test.assert(FLAG_BIT_ARRAY[0], static_cast<uint8_t>(0),
                "key item unlock leaves script flags untouched");

    item::Inventory inventory;
    item::inventoryClear(inventory);
    const uint8_t *raw = reinterpret_cast<const uint8_t *>(&inventory);
    for (uint8_t i = 0; i < sizeof(inventory); ++i) {
        test.assert(raw[i], static_cast<uint8_t>(0), "clear zeros all inventory bytes");
    }
    test.assert(item::inventoryAdd(inventory, item::ItemKind::Lure, 0, 100),
                static_cast<uint8_t>(99), "add clamps to count max and reports amount added");
    test.assert(item::inventoryCount(inventory, item::ItemKind::Lure, 0),
                static_cast<uint8_t>(99), "count observes clamped lure stack");
    test.assert(item::inventoryAdd(inventory, item::ItemKind::Lure, 0, 1),
                static_cast<uint8_t>(0), "add on full stack reports zero");
    test.assert(item::inventoryTake(inventory, item::ItemKind::Lure, 0, 100), false,
                "take more than held fails");
    test.assert(item::inventoryCount(inventory, item::ItemKind::Lure, 0),
                static_cast<uint8_t>(99), "failed take leaves count unchanged");
    test.assert(item::inventoryCount(inventory, item::ItemKind::Lure, 24),
                static_cast<uint8_t>(0), "count rejects out of range lure id");
    test.assert(item::inventoryAdd(inventory, item::ItemKind::Lure, 24, 1),
                static_cast<uint8_t>(0), "add rejects out of range lure id");
    test.assert(item::inventoryTake(inventory, item::ItemKind::Lure, 24, 1), false,
                "take rejects out of range lure id");
    test.assert(item::inventoryCount(inventory, item::ItemKind::Key, 0),
                static_cast<uint8_t>(0), "count rejects key item kind");
    test.assert(item::inventoryAdd(inventory, item::ItemKind::Key, 0, 1),
                static_cast<uint8_t>(0), "add rejects key item kind");
    test.assert(item::inventoryTake(inventory, item::ItemKind::Key, 0, 1), false,
                "take rejects key item kind");

    item::inventoryClear(inventory);
    item::inventoryAdd(inventory, item::ItemKind::Consumable, 5, 1);
    item::inventoryAdd(inventory, item::ItemKind::Lure, 7, 1);
    item::inventoryAdd(inventory, item::ItemKind::Lure, 2, 1);
    item::inventoryAdd(inventory, item::ItemKind::Consumable, 1, 1);
    test.assert(item::inventoryNonZeroCount(inventory), static_cast<uint8_t>(4),
                "nonzero count counts occupied ids");
    item::ItemKind kind;
    uint8_t id = 0;
    test.assert(item::inventoryNthNonZero(inventory, 0, kind, id), true,
                "first nonzero entry exists");
    test.assert(kind, item::ItemKind::Lure, "walk lists lures before consumables");
    test.assert(id, static_cast<uint8_t>(2), "walk sorts lure ids ascending");
    item::inventoryNthNonZero(inventory, 1, kind, id);
    test.assert(kind, item::ItemKind::Lure, "second entry is next lure");
    test.assert(id, static_cast<uint8_t>(7), "second lure id is ascending");
    item::inventoryNthNonZero(inventory, 2, kind, id);
    test.assert(kind, item::ItemKind::Consumable, "consumables follow lures");
    test.assert(id, static_cast<uint8_t>(1), "consumable ids are ascending");
    item::inventoryNthNonZero(inventory, 3, kind, id);
    test.assert(id, static_cast<uint8_t>(5), "last entry is next consumable");
    test.assert(item::inventoryNthNonZero(inventory, 4, kind, id), false,
                "walk refuses index past end");

    fxDataFake::readCount = 0;
    item::ItemName name = item::itemNameAddr(item::ItemKind::Lure, 0);
    test.assert(name.parts, static_cast<uint8_t>(2), "first lure has two parts");
    test.assert(name.part[0], LureTierNames::LureTierNames, "first lure tier uses tier table index zero");
    test.assert(name.part[1], LureTypeNames::LureTypeNames, "first lure type follows tier");
    test.assert(fxDataFake::readCount, static_cast<uint32_t>(2), "lure resolves exactly two FX entries");

    name = item::itemNameAddr(item::ItemKind::Lure, 23);
    test.assert(name.parts, static_cast<uint8_t>(2), "last lure has two parts");
    test.assert(name.part[0], LureTierNames::LureTierNames + 2 * 3, "last lure uses tier two with three-byte stride");
    test.assert(name.part[1], LureTypeNames::LureTypeNames + 7 * 3, "last lure uses type seven with three-byte stride");

    fxDataFake::readCount = 0;
    name = item::itemNameAddr(item::ItemKind::Consumable, 0);
    test.assert(name.parts, static_cast<uint8_t>(1), "first consumable has one part");
    test.assert(name.part[0], ConsumableNames::ConsumableNames, "consumable zero is a real name");
    test.assert(name.part[1], static_cast<uint24_t>(0), "unused consumable part is zero");
    test.assert(fxDataFake::readCount, static_cast<uint32_t>(1), "consumable resolves one FX entry");
    name = item::itemNameAddr(item::ItemKind::Consumable, 7);
    test.assert(name.part[0], ConsumableNames::ConsumableNames + 7 * 3, "last consumable uses three-byte stride");

    fxDataFake::readCount = 0;
    test.assert(item::itemNameAddr(item::ItemKind::Lure, item::LURE_COUNT).parts,
                static_cast<uint8_t>(0), "invalid lure has no name");
    test.assert(item::itemNameAddr(item::ItemKind::Consumable, item::CONSUMABLE_COUNT).parts,
                static_cast<uint8_t>(0), "invalid consumable has no name");
    test.assert(item::itemNameAddr(item::ItemKind::Lure, 255).parts,
                static_cast<uint8_t>(0), "large lure id has no name");
    test.assert(item::itemNameAddr(item::ItemKind::Consumable, 255).parts,
                static_cast<uint8_t>(0), "large consumable id has no name");
    test.assert(item::itemNameAddr(item::ItemKind::Key, 0).parts,
                static_cast<uint8_t>(0), "key names are unsupported");
    test.assert(item::itemNameAddr(static_cast<item::ItemKind>(255), 0).parts,
                static_cast<uint8_t>(0), "unknown kind has no name");
    test.assert(fxDataFake::readCount, static_cast<uint32_t>(0), "invalid names perform no FX reads");

    suite.addTest(test);
    runner.addTestSuite(suite);
}
