#pragma once

#include "test.hpp"
#include "src/WorldInteractionFake.hpp"
#include "../src/engine/world/World.hpp"
#include "../src/engine/world/TileProps.hpp"
#include "../src/engine/world/TilePropertyWindow.hpp"
#include "../src/engine/world/LureZone.hpp"
#include "../src/item/Inventory.hpp"
#include "../src/item/ItemIds.hpp"
#include "../src/fxdata.h"
#include "../src/lib/Effect.hpp"
#include "../src/creature/LevelCurve.hpp"
#include "../src/save/Compaction.hpp"
#include "../src/save/FlashBackend.hpp"
#include "../src/save/Journal.hpp"
#include "../src/save/SaveController.hpp"
#include "../src/globals.hpp"
#include <cstring>

namespace {
// Seed the RAM property window the way the render path would, so the faced
// tile resolves to the given properties without any FX read on the A press.
void seedFacedProps(WorldTransient &world, int16_t x, int16_t y, uint8_t props) {
    TilePropertyWindow window(world.propertyWindow);
    window.begin(x - 1, y - 1);
    uint16_t row[TilePropertyWindow::WINDOW_WIDTH] = {};
    row[1] = TileProps::packTile(9, props);
    window.writeRow(1, row);
}
}

void WorldInteractionSuite(TestRunner &runner) {
    TestSuite suite = TestSuite("World Interaction");
    Test test = Test(__func__);
    dialogMenu.clear();
    worldInteractionFake::reset();

    gameState.playerLocation = static_cast<uint16_t>((64U << 8) | 7U);
    WorldEngine::init(worldState());
    worldState().motion.directionAndFlags = static_cast<uint8_t>(Direction::RIGHT);
    worldInteractionFake::slotBytes[0] = 0xA5;
    worldInteractionFake::pressedA = false;
    WorldEngine::interact();
    test.assert(worldInteractionFake::readCount, static_cast<uint16_t>(0),
                "A release does not read a script slot");

    worldInteractionFake::pressedA = true;
    WorldEngine::interact();
    const uint16_t playerTile = static_cast<uint16_t>((64U << 8) | 7U);
    const uint16_t playerChunk = Chunk::chunkOfLocation(playerTile);
    test.assert(playerChunk, static_cast<uint16_t>(512), "player location selects chunk 512");
    test.assert(worldInteractionFake::readCount, static_cast<uint16_t>(1),
                "one A trigger performs one slot read");
    test.assert(worldInteractionFake::lastAddress,
                Chunk::scriptSlotAddr(worldInteractionFake::scriptsBase, playerChunk),
                "slot address comes from player chunk");
    test.assert(worldInteractionFake::lastAddress > 0x010000, true,
                "high script slot address remains above 16 bits");
    test.assert(worldInteractionFake::lastLength, static_cast<uint8_t>(128),
                "trigger reads exactly one full slot");
    test.assert(worldInteractionFake::lastScript == worldState().script, true,
                "slot read targets the world union buffer");
    test.assert(worldInteractionFake::runFirstByte, static_cast<uint8_t>(0xA5),
                "VM receives bytes injected into the world buffer");
    test.assert(worldState().motion.directionAndFlags,
                static_cast<uint8_t>(Direction::RIGHT),
                "interaction restores persistent facing after slot use");
    test.assert(worldInteractionFake::currentTile, playerTile,
                "VM receives the player tile");
    test.assert(worldInteractionFake::targetTile,
                static_cast<uint16_t>((64U << 8) | 8U),
                "right-facing target increments x without changing row");

    worldInteractionFake::reset();
    dialogMenu.clear();
    gameState.playerLocation = static_cast<uint16_t>((64U << 8) | 15U);
    WorldEngine::init(worldState());
    worldState().motion.directionAndFlags = static_cast<uint8_t>(Direction::UP);
    worldInteractionFake::pressedA = true;
    WorldEngine::interact();
    test.assert(worldInteractionFake::readCount, static_cast<uint16_t>(1),
                "up-facing interaction reads a slot");
    test.assert(worldInteractionFake::targetTile,
                static_cast<uint16_t>((63U << 8) | 15U),
                "up-facing target decrements y");

    worldInteractionFake::reset();
    gameState.playerLocation = static_cast<uint16_t>((64U << 8) | 7U);
    WorldEngine::init(worldState());
    worldState().motion.directionAndFlags = static_cast<uint8_t>(Direction::DOWN);
    worldInteractionFake::pressedA = true;
    WorldEngine::interact();
    test.assert(worldInteractionFake::readCount, static_cast<uint16_t>(1),
                "down-facing interaction reads a slot");
    test.assert(worldInteractionFake::targetTile,
                static_cast<uint16_t>((65U << 8) | 7U),
                "down-facing target increments y");

    worldInteractionFake::reset();
    gameState.playerLocation = static_cast<uint16_t>((64U << 8) | 7U);
    WorldEngine::init(worldState());
    worldState().motion.directionAndFlags = static_cast<uint8_t>(Direction::LEFT);
    worldInteractionFake::pressedA = true;
    WorldEngine::interact();
    test.assert(worldInteractionFake::readCount, static_cast<uint16_t>(1),
                "left-facing interaction reads a slot");
    test.assert(worldInteractionFake::targetTile,
                static_cast<uint16_t>((64U << 8) | 6U),
                "left-facing target decrements x");

    worldInteractionFake::reset();
    gameState.playerLocation = 0;
    WorldEngine::init(worldState());
    worldState().motion.directionAndFlags = static_cast<uint8_t>(Direction::LEFT);
    worldInteractionFake::pressedA = true;
    WorldEngine::interact();
    test.assert(worldInteractionFake::readCount, static_cast<uint16_t>(0),
                "signed edge bounds reject a wrapped left target");

    worldInteractionFake::reset();
    gameState.playerLocation = static_cast<uint16_t>((20U << 8) | 20U);
    WorldEngine::init(worldState());
    worldState().motion.directionAndFlags = static_cast<uint8_t>(Direction::DOWN);
    PopUpDialog active = {};
    active.type = SCRIPT_TEXT;
    active.textAddress = 1;
    dialogMenu.pushMenu(active);
    worldInteractionFake::pressedA = true;
    WorldEngine::interact();
    test.assert(worldInteractionFake::readCount, static_cast<uint16_t>(0),
                "open dialog prevents retriggering a script");
    WorldEngine::runMap(worldState());
    test.assert(dialogMenu.peek(), false, "A pops an active dialog");
    test.assert(worldInteractionFake::readCount, static_cast<uint16_t>(0),
                "A-pop does not read another script slot");

    // Box and plant hooks resolve the faced tile from the RAM property window,
    // so they add no FX reads on top of the single VM slot read. Other targets
    // keep the existing single slot read.
    dialogMenu.clear();
    LureZone::closePicker();
    item::inventoryClear(LureZone::inventory());
    worldInteractionFake::reset();
    gameState.playerLocation = static_cast<uint16_t>((64U << 8) | 7U);
    WorldEngine::init(worldState());
    worldState().motion.directionAndFlags = static_cast<uint8_t>(Direction::RIGHT);
    seedFacedProps(worldState(), 8, 64, TileProps::PROP_SPARE);
    worldInteractionFake::pressedA = true;
    WorldEngine::interact();
    test.assert(LureZone::pickerOpen(), true,
                "spare-prop box opens the material picker");
    test.assert(worldInteractionFake::readCount, static_cast<uint16_t>(0),
                "box target skips the VM script slot");

    // --- First-beta renewable plant plot (jp8.11.5) -------------------------
    // The plant hook reads its faced tile from the RAM property window like the
    // box, so every plant A press adds no FX read. Drive the authored plot index
    // through plant/grow/harvest: an empty plot plants, a growing plot is inert,
    // a mature plot yields one authored tier-1 lure (item::lureId(0,1)) and
    // resets to planted, and a full bag holds maturity with the lureTextFull
    // feedback so a retry can harvest once the bag has room.
    auto facePlant = []() {
        gameState.playerLocation = static_cast<uint16_t>((64U << 8) | 7U);
        WorldEngine::init(worldState());
        worldState().motion.directionAndFlags = static_cast<uint8_t>(Direction::RIGHT);
        seedFacedProps(worldState(), 8, 64, TileProps::PROP_PLANTABLE);
    };
    auto pressPlant = []() {
        worldInteractionFake::reset();
        worldInteractionFake::pressedA = true;
        WorldEngine::interact();
    };
    auto plantAges = [](uint8_t increments) {
        for (uint8_t i = 0; i < increments; ++i)
            plants.plantStages.increment(FIRST_BETA_PLOT_INDEX);
    };

    // Empty plot: A plants stage1 at no item cost and reads no script slot.
    dialogMenu.clear();
    LureZone::closePicker();
    plants = PlantGameState{};
    item::inventoryClear(LureZone::inventory());
    facePlant();
    pressPlant();
    test.assert(plants.plantStages.getStage(FIRST_BETA_PLOT_INDEX),
                static_cast<uint8_t>(1), "A on an empty plot plants stage1");
    test.assert(item::inventoryCount(LureZone::inventory(), item::ItemKind::Lure,
                                     item::lureId(0, 1)),
                static_cast<uint8_t>(0), "planting costs no lure");
    test.assert(worldInteractionFake::readCount, static_cast<uint16_t>(0),
                "empty plant target skips the VM script slot");

    // Growing plot: A is inert (no item, stage held).
    plants = PlantGameState{};
    plantAges(2);
    facePlant();
    pressPlant();
    test.assert(plants.plantStages.getStage(FIRST_BETA_PLOT_INDEX),
                static_cast<uint8_t>(2), "A on a growing plot is inert");
    test.assert(item::inventoryCount(LureZone::inventory(), item::ItemKind::Lure,
                                     item::lureId(0, 1)),
                static_cast<uint8_t>(0), "growing plot grants nothing");
    test.assert(worldInteractionFake::readCount, static_cast<uint16_t>(0),
                "growing plant target skips the VM script slot");

    // Mature plot: A harvests one authored tier-1 lure, resets to planted, and
    // shows the gathered feedback. The retired base lure is not granted.
    plants = PlantGameState{};
    plantAges(3);
    facePlant();
    pressPlant();
    test.assert(item::inventoryCount(LureZone::inventory(), item::ItemKind::Lure,
                                     item::lureId(0, 1)),
                static_cast<uint8_t>(1), "mature harvest grants one tier-1 lure");
    test.assert(item::inventoryCount(LureZone::inventory(), item::ItemKind::Lure,
                                     item::lureId(0, 0)),
                static_cast<uint8_t>(0), "harvest no longer grants the base lure");
    test.assert(plants.plantStages.getStage(FIRST_BETA_PLOT_INDEX),
                static_cast<uint8_t>(1), "harvest resets the plot to planted");
    test.assert(LureZone::messageAddr(), lureTextGathered,
                "harvest shows the gathered feedback");
    test.assert(worldInteractionFake::readCount, static_cast<uint16_t>(0),
                "mature harvest skips the VM script slot");

    // Full bag: maturity holds for a retry and the bag-full feedback shows; the
    // count stays at max. With room again the same mature plot harvests.
    plants = PlantGameState{};
    plantAges(3);
    item::inventoryClear(LureZone::inventory());
    item::inventoryAdd(LureZone::inventory(), item::ItemKind::Lure,
                       item::lureId(0, 1), item::COUNT_MAX);
    facePlant();
    pressPlant();
    test.assert(item::inventoryCount(LureZone::inventory(), item::ItemKind::Lure,
                                     item::lureId(0, 1)),
                static_cast<uint8_t>(item::COUNT_MAX), "full bag leaves the count at max");
    test.assert(plants.plantStages.getStage(FIRST_BETA_PLOT_INDEX),
                static_cast<uint8_t>(3), "full bag holds the mature plot for retry");
    test.assert(LureZone::messageAddr(), lureTextFull,
                "full bag shows the bag-full feedback");
    test.assert(worldInteractionFake::readCount, static_cast<uint16_t>(0),
                "full bag harvest skips the VM script slot");

    item::inventoryClear(LureZone::inventory());
    item::inventoryAdd(LureZone::inventory(), item::ItemKind::Lure,
                       item::lureId(0, 1), static_cast<uint8_t>(item::COUNT_MAX - 1));
    pressPlant();
    test.assert(item::inventoryCount(LureZone::inventory(), item::ItemKind::Lure,
                                     item::lureId(0, 1)),
                static_cast<uint8_t>(item::COUNT_MAX), "retry after room harvests");
    test.assert(plants.plantStages.getStage(FIRST_BETA_PLOT_INDEX),
                static_cast<uint8_t>(1), "retry resets the plot to planted");
    test.assert(LureZone::messageAddr(), lureTextGathered,
                "retry shows the gathered feedback");
    item::inventoryClear(LureZone::inventory());

    worldInteractionFake::reset();
    gameState.playerLocation = static_cast<uint16_t>((64U << 8) | 7U);
    WorldEngine::init(worldState());
    worldState().motion.directionAndFlags = static_cast<uint8_t>(Direction::RIGHT);
    worldInteractionFake::pressedA = true;
    WorldEngine::interact();
    test.assert(worldInteractionFake::readCount, static_cast<uint16_t>(1),
                "ordinary target keeps the VM script read");

    // --- First-beta free rest (faced tile 10,4) -----------------------------
    // The rest point is a faced-tile coordinate trigger, not a tile prop or map
    // marker: the player stands at (10,5) facing UP so the target is the
    // authored rest_x/rest_y (10,4) from data/beta-area.toml.
    auto faceRest = []() {
        gameState.playerLocation = static_cast<uint16_t>((5U << 8) | 10U);
        WorldEngine::init(worldState());
        worldState().motion.directionAndFlags = static_cast<uint8_t>(Direction::UP);
    };

    // Empty party: rest is a no-op and reads no script slot.
    dialogMenu.clear();
    LureZone::closePicker();
    worldInteractionFake::reset();
    player = Player();
    for (uint8_t i = 0; i < 3; ++i) {
        player.party[i].id = 255;
        player.creatureHPs[i] = 0;
    }
    faceRest();
    worldInteractionFake::pressedA = true;
    WorldEngine::interact();
    test.assert(worldInteractionFake::readCount, static_cast<uint16_t>(0),
                "rest target skips the VM script slot");
    for (uint8_t i = 0; i < 3; ++i)
        test.assert(player.creatureHPs[i], static_cast<uint8_t>(0),
                    "empty party slot stays empty after rest");

    // One occupied member: only that slot heals to its derived maxHP.
    worldInteractionFake::reset();
    player = Player();
    for (uint8_t i = 0; i < 3; ++i) {
        player.party[i].id = 255;
        player.creatureHPs[i] = 0;
    }
    player.loadCreature(0, 0);
    player.creatureHPs[0] = 0;
    faceRest();
    worldInteractionFake::pressedA = true;
    WorldEngine::interact();
    test.assert(player.creatureHPs[0], player.party[0].hp,
                "single KO member heals to derived maxHP");
    test.assert(player.creatureHPs[1], static_cast<uint8_t>(0),
                "unoccupied slot is not touched");
    test.assert(player.creatureHPs[2], static_cast<uint8_t>(0),
                "second unoccupied slot is not touched");

    // Three occupied, partial KO: all heal and transient status clears.
    worldInteractionFake::reset();
    player = Player();
    player.loadCreature(0, 0);
    player.loadCreature(1, 3);
    player.loadCreature(2, 6);
    player.creatureHPs[0] = 0;
    player.creatureHPs[1] = 1;
    player.creatureHPs[2] = static_cast<uint8_t>(player.party[2].hp - 1);
    player.party[1].status.applyEffect(Effect::SAPPD);
    faceRest();
    worldInteractionFake::pressedA = true;
    WorldEngine::interact();
    for (uint8_t i = 0; i < 3; ++i)
        test.assert(player.creatureHPs[i], player.party[i].hp,
                    "every occupied member rests to full HP");
    test.assert(static_cast<uint8_t>(player.party[1].status.effects[0]),
                static_cast<uint8_t>(Effect::NONE),
                "rest clears transient status");

    // Repeated rest is idempotent and still reads no script slot.
    worldInteractionFake::reset();
    worldInteractionFake::pressedA = true;
    WorldEngine::interact();
    for (uint8_t i = 0; i < 3; ++i)
        test.assert(player.creatureHPs[i], player.party[i].hp,
                    "repeated rest keeps full HP");
    test.assert(worldInteractionFake::readCount, static_cast<uint16_t>(0),
                "repeated rest reads no script slot");

    // Explicit checkpoint mirrors rested HP; a boxed instance never heals and
    // rest itself consumes no journal capacity.
    flashFakeReset();
    storeFresh();
    player = Player();
    player.basic();
    SaveController::seedParty();
    player.creatureHPs[0] = 0;
    player.creatureHPs[1] = 1;
    player.creatureHPs[2] = static_cast<uint8_t>(player.party[2].hp - 1);
    StoreRecord boxed = {};
    boxed.id = player.party[0].id;
    boxed.exp = levelExpThreshold(player.party[0].level);
    boxed.hpEncoded = 1; // KO boxed instance
    uint8_t boxedSlot = 255;
    test.assert(storeAdd(boxed, boxedSlot), true, "seed boxed KO instance");
    test.assert(storeInstanceHP(boxed), 0, "boxed instance starts KO");
    const uint8_t journalBefore = journalCount();
    faceRest();
    worldInteractionFake::reset();
    worldInteractionFake::pressedA = true;
    WorldEngine::interact();
    for (uint8_t i = 0; i < 3; ++i)
        test.assert(player.creatureHPs[i], player.party[i].hp,
                    "rest heals the live party before the checkpoint");
    test.assert(journalCount(), journalBefore,
                "rest consumes no journal capacity");
    SaveController::begin(GameState_t::WORLD);
    for (uint16_t i = 0; i < 256 && saveInProgress(); ++i) SaveController::advance();
    test.assert(saveInProgress(), false, "rest checkpoint completes");
    for (uint8_t i = 0; i < 3; ++i) player.creatureHPs[i] = 0;
    test.assert(SaveController::load(), true, "rested checkpoint reloads");
    for (uint8_t i = 0; i < 3; ++i)
        test.assert(player.creatureHPs[i], player.party[i].hp,
                    "reloaded party keeps rested full HP");
    StoreRecord restored = {};
    test.assert(storeRead(boxedSlot, restored), true, "boxed instance reloads");
    test.assert(storeInstanceHP(restored), 0,
                "boxed instance never heals from rest");

    // Step 3: a refused instance-record write must not be claimed. Saturate the
    // live pending index so any fresh record write is refused, then prove rest
    // still recovers live HP, publishes no record, and that the next explicit
    // checkpoint mirrors the recovered party HP.
    flashFakeReset();
    storeFresh();
    player = Player();
    player.basic();
    SaveController::seedParty();
    StoreRecord filler = {};
    filler.id = player.party[0].id;
    filler.exp = levelExpThreshold(player.party[0].level);
    filler.hpEncoded = 1;
    for (uint8_t n = 0; n < 13; ++n) {
        uint8_t filled = 255;
        test.assert(storeAdd(filler, filled), true, "fill pending record index");
    }
    StoreRecord refused = filler;
    test.assert(storeWrite(16, refused), false,
                "a fresh instance write is refused at capacity");
    const uint8_t journalAtCapacity = journalCount();
    player.creatureHPs[0] = 0;
    player.creatureHPs[1] = 1;
    player.creatureHPs[2] = static_cast<uint8_t>(player.party[2].hp - 1);
    faceRest();
    worldInteractionFake::reset();
    worldInteractionFake::pressedA = true;
    WorldEngine::interact();
    for (uint8_t i = 0; i < 3; ++i)
        test.assert(player.creatureHPs[i], player.party[i].hp,
                    "rest recovers live HP while a write would be refused");
    test.assert(journalCount(), journalAtCapacity,
                "rest publishes no instance record under refusal");
    test.assert(storeWrite(16, refused), false,
                "rest leaves the record write still refused");
    SaveController::begin(GameState_t::WORLD);
    for (uint16_t i = 0; i < 256 && saveInProgress(); ++i) SaveController::advance();
    test.assert(saveInProgress(), false, "checkpoint after refusal completes");
    for (uint8_t i = 0; i < 3; ++i) player.creatureHPs[i] = 0;
    test.assert(SaveController::load(), true, "checkpoint after refusal reloads");
    for (uint8_t i = 0; i < 3; ++i)
        test.assert(player.creatureHPs[i], player.party[i].hp,
                    "next explicit checkpoint mirrors recovered HP");

    dialogMenu.clear();
    worldInteractionFake::pressedA = false;
    suite.addTest(test);
    runner.addTestSuite(suite);
}
