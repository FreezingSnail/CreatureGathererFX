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


#ifdef CGFX_LURE_WEDGE
    test.expectEq(lureWorldTilesFrames,96,F("lure scenery has96 render cells"));
    uint16_t mapHeadroom=65535;
    for(uint8_t d=0;d<5;++d) {
        gameState.playerLocation=static_cast<uint16_t>((5u<<8)|11u);
        WorldEngine::init(worldState());
        worldState().motion.directionAndFlags=d<4?d:0;
        worldState().motion.step=d<4?4:0;
        // Match the motion mask used by normal16px movement.
        if(d<4) worldState().motion.directionAndFlags |= static_cast<uint8_t>((d==0?3:d==1?2:d==2?4:1)<<3);
        const ViewOffset offset=WorldEngine::view(worldState());
        memset(Arduboy2Base::sBuffer,0,1024);
        const uint16_t base=reinterpret_cast<uint16_t>(&stack_fx_test_detail::__bss_end);
        const uint16_t top=stack_fx_test_detail::paintStack();
        drawMapFast(worldState());
        const uint16_t headroom=stack_fx_test_detail::lowWater(base,top)-base;
        if(headroom<mapHeadroom)mapHeadroom=headroom;
        bool matches=true;
        for(uint8_t y=0;y<64;++y)for(uint8_t x=0;x<128;++x) {
            const int16_t wx=128+x-offset.x,wy=48+y-offset.y;
            const uint8_t mx=wx>>4,my=wy>>4;
            uint24_t address;
            uint16_t frame;
            if(mx>=4&&mx<16&&my<8){address=lureWorldTiles+4;frame=my*12+mx-4;}
            else {
                uint16_t word;
                FX::readDataBytes(raw_map_data+(static_cast<uint24_t>(my)*256+mx)*2,reinterpret_cast<uint8_t *>(&word),2);
                if(TileProps::tileId(word)==0) {
                    if((Arduboy2Base::sBuffer[(y>>3)*128+x]>>(y&7))&1)matches=false;
                    continue;
                }
                frame=TileProps::tileId(word)-1;address=tiles;
            }
            uint8_t byte;
            FX::readDataBytes(address+static_cast<uint24_t>(frame)*32+((wy&15)>>3)*16+(wx&15),&byte,1);
            const uint8_t expected=(byte>>(wy&7))&1;
            const uint8_t actual=(Arduboy2Base::sBuffer[(y>>3)*128+x]>>(y&7))&1;
            if(expected!=actual)matches=false;
        }
        test.expectEq(matches,true,F("lure map idle and scrolling match packed art"));
    }
    test.expectEq(mapHeadroom>=stack_fx_test_detail::MIN_HEADROOM,true,F("lure map render preserves stack reserve"));
    Serial.print(F("lure map headroom="));Serial.println(mapHeadroom);
#endif

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
