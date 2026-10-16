#include "harness/fx_globals.hpp"
#include "src/engine/draw.h"
#include "src/engine/world/StepEvent.hpp"
#include "src/engine/world/LureZone.hpp"
#include "src/engine/battle/BattleFlow.hpp"
#include "src/engine/battle/DuelView.hpp"
#include "src/item/Inventory.hpp"
#include "src/item/ItemIds.hpp"
#include "fxtest.hpp"
#include "frame_budget_test.hpp"
#include "stack_test.hpp"

// WorldEngine::runMap reaches the world VM; the shipping sketch defines this
// global, so this suite must too once it drives a world frame.
ScriptVm vm;

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

    // First-beta free rest and defeat recovery share Player::healParty on the
    // actual device. Prove the occupied-party restore, the authored start
    // return and that an empty slot is never touched.
    player.basic();
    const uint8_t maxHp0 = player.party[0].statlist.hp;
    const uint8_t maxHp1 = player.party[1].statlist.hp;
    const uint8_t maxHp2 = player.party[2].statlist.hp;
    player.creatureHPs[0] = 0;
    player.creatureHPs[1] = 1;
    player.creatureHPs[2] = static_cast<uint8_t>(maxHp2 - 1);
    gameState.state = GameState_t::WORLD;
    gameState.playerLocation = static_cast<uint16_t>((9u << 8) | 11u);
    WorldEngine::queueDefeatRecovery();
    WorldEngine::runMap(worldState());
    test.expectEq(player.creatureHPs[0], maxHp0, F("defeat recovery heals ko member"));
    test.expectEq(player.creatureHPs[1], maxHp1, F("defeat recovery heals injured member"));
    test.expectEq(player.creatureHPs[2], maxHp2, F("defeat recovery heals partial member"));
    test.expectEq(WorldEngine::location(), static_cast<uint16_t>((5u << 8) | 11u),
                  F("defeat recovery returns to beta start"));
    WorldEngine::runMap(worldState());
    test.expectEq(WorldEngine::location(), static_cast<uint16_t>((5u << 8) | 11u),
                  F("defeat recovery does not retrigger movement"));

    player.party[2].id = 255;
    player.creatureHPs[2] = 7;
    player.healParty();
    test.expectEq(player.creatureHPs[2], 7, F("rest skips an empty party slot"));


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

    // Frame-budget proof on the real world step: runMap + map/player/overlay
    // render + the real FX display, using the shared micros() instrumentation.
    // The deliberately slowed frame proves the assertion is non-vacuous.
    {
        frame_budget_test_detail::FrameBudget budget;
        budget.reset();
        budget.armSlowFrame();
        budget.begin();
        WorldEngine::runMap(worldState());
        drawMapFast(worldState());
        drawPlayer(worldState());
        LureZone::drawOverlay();
        FX::display(CLEAR_BUFFER);
        budget.end();
        budget.expectDetectsSlowFrame(
            test, F("negative control: slowed world frame exceeds budget"));

        budget.reset();
        for (uint8_t frame = 0; frame < 8 && gameState.state == GameState_t::WORLD; ++frame) {
            budget.begin();
            WorldEngine::runMap(worldState());
            drawMapFast(worldState());
            drawPlayer(worldState());
            LureZone::drawOverlay();
            FX::display(CLEAR_BUFFER);
            budget.end();
        }
        test.expectEq(gameState.state == GameState_t::WORLD, true,
                      F("world frames stay in the world state"));
        budget.expectWithinBudget(test, F("worst world frame <= 19230 us"));
    }

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

    // Gap 2 (CreatureGathererFX-o845): drive the live encounter through a full
    // battle and a non-Lose exit, then prove the world resumes on the exact
    // pre-battle tile with the battle HP persisted. The matchup is pinned so the
    // round is a known spirit-over-fire trade (21 to the player, 4 to the foe)
    // that cannot terminate the battle.
    const uint16_t preBattleLocation = WorldEngine::location();
    test.expectEq(preBattleLocation, static_cast<uint16_t>((7u << 8) | 12u),
                  F("encounter pinned the pre-battle tile"));

    battle::BattleState &duel =
        const_cast<battle::BattleState &>(battleSession().state());
    duel.active[0].types = DualType(Type::FIRE);
    duel.active[0].hp = duel.active[0].maxHp = 100;
    duel.active[0].attackMask =
        static_cast<uint8_t>(1u << duel::foldType(static_cast<uint8_t>(Type::FIRE)));
    duel.active[1].types = DualType(Type::FIRE);
    duel.active[1].hp = duel.active[1].maxHp = 100;
    duel.active[1].attackMask =
        static_cast<uint8_t>(1u << duel::foldType(static_cast<uint8_t>(Type::SPIRIT)));

    BattleFlow::update(0);           // choice entry -> pick row
    BattleFlow::update(MENU_EDGE_A); // commit the single attack cell

    uint16_t roundFrames = 0;
    while (!battleSession().awaitingPlayer() &&
           gameState.state == GameState_t::BATTLE && roundFrames < 400) {
        BattleFlow::update(0);
        ++roundFrames;
    }
    test.expectEq(gameState.state == GameState_t::BATTLE, true,
                  F("the trade round stays in battle"));
    test.expectEq(battleSession().awaitingPlayer(), true,
                  F("the trade round returns to a fresh choice"));
    test.expectEq(battleSession().state().active[0].hp, 79,
                  F("the trade round damaged the player active"));
    const uint8_t damagedHp = battleSession().state().active[0].hp;

    BattleFlow::update(MENU_EDGE_B); // wild action page (ESCAPE only, not gatherable)
    BattleFlow::update(MENU_EDGE_A); // ESCAPE -> immediate non-Lose result

    uint16_t exitFrames = 0;
    while (gameState.state != GameState_t::WORLD && exitFrames < 400) {
        BattleFlow::update(0);
        ++exitFrames;
    }
    test.expectEq(gameState.state, GameState_t::WORLD,
                  F("a non-Lose exit returns to the world"));
    test.expectEq(WorldEngine::location(), preBattleLocation,
                  F("the world resumes on the exact pre-battle tile"));
    test.expectEq(player.creatureHPs[0], damagedHp,
                  F("the battle HP persists into the world party"));
    test.expectEq(player.creatureHPs[0], 79,
                  F("the persisted HP is the damaged value, not a reset"));

    test.report(F("test_world_player"));
}
void loop() { exit(0); }
