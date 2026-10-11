#pragma once

#include "test.hpp"
#include "src/WorldInteractionFake.hpp"
#include "../src/engine/world/World.hpp"
#include "../src/engine/world/TileProps.hpp"
#include "../src/engine/world/TilePropertyWindow.hpp"
#include "../src/engine/world/LureZone.hpp"
#include "../src/item/Inventory.hpp"
#include "../src/item/ItemIds.hpp"
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

    LureZone::closePicker();
    worldInteractionFake::reset();
    gameState.playerLocation = static_cast<uint16_t>((64U << 8) | 7U);
    WorldEngine::init(worldState());
    worldState().motion.directionAndFlags = static_cast<uint8_t>(Direction::RIGHT);
    seedFacedProps(worldState(), 8, 64, TileProps::PROP_PLANTABLE);
    worldInteractionFake::pressedA = true;
    WorldEngine::interact();
    test.assert(item::inventoryCount(LureZone::inventory(), item::ItemKind::Lure,
                                     item::lureId(0, 0)),
                static_cast<uint8_t>(1), "plant face gathers one base lure");
    test.assert(worldInteractionFake::readCount, static_cast<uint16_t>(0),
                "plant target skips the VM script slot");

    worldInteractionFake::reset();
    gameState.playerLocation = static_cast<uint16_t>((64U << 8) | 7U);
    WorldEngine::init(worldState());
    worldState().motion.directionAndFlags = static_cast<uint8_t>(Direction::RIGHT);
    worldInteractionFake::pressedA = true;
    WorldEngine::interact();
    test.assert(worldInteractionFake::readCount, static_cast<uint16_t>(1),
                "ordinary target keeps the VM script read");

    dialogMenu.clear();
    worldInteractionFake::pressedA = false;
    suite.addTest(test);
    runner.addTestSuite(suite);
}
