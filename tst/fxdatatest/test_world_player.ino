#include "harness/fx_globals.hpp"
#include "src/engine/draw.h"
#include "src/engine/world/StepEvent.hpp"
#include "src/engine/world/LureZone.hpp"
#include "src/item/Inventory.hpp"
#include "src/item/ItemIds.hpp"
#include "fxtest.hpp"
#include "stack_test.hpp"

namespace lureWedgeTest {
uint8_t forcedRoll = 0;
uint8_t forcedRng(uint8_t, uint8_t) { return forcedRoll; }
}

void setup() {
    fxTestSetup();
    FxTest test;
    test.expectEq(worldPlayerSpritesWidth, 16, F("player width"));
    test.expectEq(worldPlayerSpritesHeight, 16, F("player height"));
    test.expectEq(worldPlayerSpritesFrames, 12, F("four directions times three poses"));
    uint16_t minimumHeadroom = 65535;
    for (uint8_t d = 0; d < 4; ++d) for (uint8_t pose = 0; pose < 3; ++pose) {
        WorldTransient &world = worldState();
        world.activateMotion(); world.motion.directionAndFlags = d;
        world.motion.step = pose == 0 ? 0 : pose == 1 ? 1 : 8;
        const uint8_t frame = d * 3 + pose;
        test.expectEq(WorldEngine::playerFrame(world), frame, F("direction and pose select packed frame"));
        memset(Arduboy2Base::sBuffer, 0xA5, 1024);
        const uint16_t base = reinterpret_cast<uint16_t>(&stack_fx_test_detail::__bss_end);
        const uint16_t top = stack_fx_test_detail::paintStack();
        drawPlayer(world);
        const uint16_t headroom = stack_fx_test_detail::lowWater(base, top) - base;
        if (headroom < minimumHeadroom) minimumHeadroom = headroom;
        bool pixelsMatch = true;
        for (uint8_t page = 0; page < 2; ++page) for (uint8_t col = 0; col < 16; ++col) {
            uint8_t bytes[2];
            FX::readDataBytes(worldPlayerSprites + 4 + static_cast<uint24_t>(frame) * 64 + (page * 16 + col) * 2, bytes, 2);
            const uint8_t expected = (0xA5 & ~bytes[1]) | (bytes[0] & bytes[1]);
            if (Arduboy2Base::sBuffer[(page + 3) * 128 + 56 + col] != expected) pixelsMatch = false;
        }
        test.expectEq(pixelsMatch, true, F("all masked columns match over terrain pattern"));
        bool outsideUnchanged = true;
        for (uint16_t i = 0; i < 1024; ++i) {
            const uint8_t page = i / 128, col = i % 128;
            if ((page < 3 || page > 4 || col < 56 || col > 71) && Arduboy2Base::sBuffer[i] != 0xA5) outsideUnchanged = false;
        }
        test.expectEq(outsideUnchanged, true, F("outside player slot unchanged"));
    }
    test.expectEq(minimumHeadroom >= stack_fx_test_detail::MIN_HEADROOM, true, F("player render preserves stack reserve"));
    Serial.print(F("player render headroom=")); Serial.println(minimumHeadroom);

    // A pocket encounter tile fires an encounter under the deterministic roll
    // seam while a lure is deposited, exercising the authored chunk-33 zone.
    item::inventoryAdd(LureZone::inventory(), item::ItemKind::Lure,
                       item::lureId(0, 0), 1);
    LureZone::clear();
    gameState.state = GameState_t::WORLD;
    gameState.playerLocation = static_cast<uint16_t>((7u << 8) | 12u);
    WorldEngine::init(worldState());
    WorldEngine::loadMap(worldState(), 0, 0);
    drawMapFast(worldState());
    LureZone::deposit(LureZone::lureId(0, 0));
    test.expectEq(LureZone::active(), true, F("pocket lure is active"));
    player.basic();
    for (uint8_t slot = 1; slot < PARTY_SIZE; ++slot) player.party[slot].level = 0;
    player.party[0].level = 5;
    lureWedgeTest::forcedRoll = 0;
    StepEventTest::setEncounterRng(lureWedgeTest::forcedRng);
    onStep(static_cast<uint16_t>((7u << 8) | 12u));
    test.expectEq(gameState.state, GameState_t::BATTLE,
                  F("pocket encounter tile fires a wild battle"));
    test.expectEq(battleSession().isActive(), true, F("encounter battle session active"));

    test.report(F("test_world_player"));
}
void loop() { exit(0); }
