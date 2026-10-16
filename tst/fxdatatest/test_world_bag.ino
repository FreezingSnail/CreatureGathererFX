#include "harness/fx_globals.hpp"
#include "fxtest.hpp"
#include "src/engine/world/LureZone.hpp"
#include "src/item/Inventory.hpp"
#include "src/item/ItemIds.hpp"
#include "src/item/KeyItems.hpp"
#include "src/player/Player.hpp"
#include "src/save/Compaction.hpp"
#include "src/save/SaveController.hpp"
#include "src/save/SaveFile.hpp"

// The single Player-owned bag is the v3 snapshot authority and the world deposit
// source (CreatureGathererFX-5j1n split out of test_world_player so that suite
// keeps its flash headroom). A picker deposit spends that bag; the remaining
// counts and the runtime key byte round-trip while the key extension bytes stay
// zero.
void setup() {
    fxTestSetup();
    FxTest test;

    player = Player();
    item::inventoryAdd(LureZone::inventory(), item::ItemKind::Lure,
                       item::lureId(0, 0), 5);
    item::inventoryAdd(LureZone::inventory(), item::ItemKind::Consumable, 4, 9);
    item::keyItemUnlock(player.keyItems, 5);
    LureZone::clear();
    LureZone::openPicker();
    test.expectEq(LureZone::pickerDeposit(), true,
                  F("world deposit spends the shared bag"));
    test.expectEq(item::inventoryCount(LureZone::inventory(), item::ItemKind::Lure,
                                       item::lureId(0, 0)), 4,
                  F("deposit consumed exactly one lure"));
    SaveController::begin(GameState_t::WORLD);
    for (uint16_t i = 0; i < 256 && saveInProgress(); ++i) {
        SaveController::advance();
        FX::waitWhileBusy();
    }
    SaveFile snap = {};
    test.expectEq(storeBoot(snap), true, F("bag snapshot boots"));
    test.expectEq(snap.lures[item::lureId(0, 0)], 4,
                  F("post-deposit lure count in snapshot"));
    test.expectEq(snap.consumables[4], 9, F("consumable count in snapshot"));
    test.expectEq(snap.keyItems[0], static_cast<uint8_t>(1u << 5),
                  F("runtime key byte in snapshot"));
    test.expectEq(snap.keyItems[1], 0, F("key extension byte stays zero"));
    player = Player();
    test.expectEq(SaveController::load(), true, F("bag snapshot loads"));
    test.expectEq(item::inventoryCount(LureZone::inventory(), item::ItemKind::Lure,
                                       item::lureId(0, 0)), 4,
                  F("lure count restored into the one bag"));
    test.expectEq(item::inventoryCount(LureZone::inventory(),
                                       item::ItemKind::Consumable, 4), 9,
                  F("consumable count restored"));
    test.expectEq(item::keyItemUnlocked(player.keyItems, 5), true,
                  F("key unlock restored"));

    test.report(F("test_world_bag"));
}
void loop() { exit(0); }
