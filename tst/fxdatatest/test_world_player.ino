#include "harness/fx_globals.hpp"
#include "src/engine/draw.h"
#include "src/engine/world/LureZone.hpp"
#include "src/engine/world/TileProps.hpp"
#include "src/engine/world/TilePropertyWindow.hpp"
#include "src/lib/FxReadCounter.hpp"
#include "src/item/Inventory.hpp"
#include "src/item/ItemIds.hpp"
#include "fxtest.hpp"
#include "frame_budget_test.hpp"
#include "stack_test.hpp"

// WorldEngine::runMap reaches the world VM; the shipping sketch defines this
// global, so this suite must too once it drives a world frame.
ScriptVm vm;

// Generated flag id (defined once by World.cpp's flags mirror include).
extern uint16_t flag_beta_complete;

// The host world/interaction/step/duel suites own the full logic matrix. This
// device suite keeps the world/player/plot/box checks that are not host-proven:
// pixel rendering, stack reserves, the real frame budget, the real A-button
// plant path with no FX metadata reads, and the faced boss trigger/ending.
static bool pocketFramePixelsMatch(uint8_t frame) {
    for (uint8_t page = 0; page < 2; ++page)
        for (uint8_t col = 0; col < 16; ++col) {
            uint8_t expected;
            FX::readDataBytes(pocketProps + 4 +
                                  static_cast<uint24_t>(frame) * 32 +
                                  page * 16 + col,
                              &expected, 1);
            if (Arduboy2Base::sBuffer[page * 128 + col] != expected) return false;
        }
    return true;
}

static bool npcSpritePixelsMatch() {
    // Player at (13,4) puts the guide at (16,4) on screen (96,32). The NPC
    // is masked over terrain, so uncovered bits retain the already-drawn map.
    for (uint8_t page = 0; page < 2; ++page)
        for (uint8_t col = 0; col < 16; ++col) {
            uint8_t bytes[2];
            FX::readDataBytes(npcSprites + 4 +
                                  static_cast<uint24_t>(page * 16 + col) * 2,
                              bytes, 2);
            const uint8_t actual =
                Arduboy2Base::sBuffer[(page + 4) * 128 + 96 + col];
            const uint8_t expected =
                (actual & ~bytes[1]) | (bytes[0] & bytes[1]);
            if (actual != expected) return false;
        }
    return true;
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
        // The player is viewport cell (3,2): pages 4-5, columns 48-63.
        for (uint8_t page = 0; page < 2; ++page) for (uint8_t col = 0; col < 16; ++col) {
            uint8_t bytes[2];
            FX::readDataBytes(worldPlayerSprites + 4 + static_cast<uint24_t>(frame) * 64 + (page * 16 + col) * 2, bytes, 2);
            const uint8_t expected = (0xA5 & ~bytes[1]) | (bytes[0] & bytes[1]);
            if (Arduboy2Base::sBuffer[(page + 4) * 128 + 48 + col] != expected) pixelsMatch = false;
        }
        test.expectEq(pixelsMatch, true, F("all masked columns match over terrain pattern"));
        bool outsideUnchanged = true;
        for (uint16_t i = 0; i < 1024; ++i) {
            const uint8_t page = i / 128, col = i % 128;
            if ((page < 4 || page > 5 || col < 48 || col > 63) && Arduboy2Base::sBuffer[i] != 0xA5) outsideUnchanged = false;
        }
        test.expectEq(outsideUnchanged, true, F("outside player slot unchanged"));
    }
    test.expectEq(minimumHeadroom >= stack_fx_test_detail::MIN_HEADROOM, true, F("player render preserves stack reserve"));
    Serial.print(F("player render headroom=")); Serial.println(minimumHeadroom);

    // Authored pocket props: the plot at (14,2) selects pocketProps frame = RAM
    // plant stage and the deposit box at (9,2) selects frame 4. Frame selection
    // logic is host-proven; this keeps the device pixel proof for one stage and
    // the box.
    test.expectEq(pocketPropsWidth, 16, F("pocket props width"));
    test.expectEq(pocketPropsHeight, 16, F("pocket props height"));
    test.expectEq(pocketPropsFrames, 5, F("four plot stages then the lure box"));
    {
        uint8_t stageBytes[8] = {};
        stageBytes[0] = 3;
        plants.plantStages.restore(stageBytes);
        // Player at (17,4) puts the authored plot (14,2) at screen (0,0).
        gameState.playerLocation = static_cast<uint16_t>((4u << 8) | 17u);
        WorldEngine::init(worldState());
        memset(Arduboy2Base::sBuffer, 0xA5, 1024);
        drawMapFast(worldState());
        test.expectEq(pocketFramePixelsMatch(3), true,
                      F("plot renders its mature pocketProps frame"));
        // Player at (12,4) puts the deposit box (9,2) at screen (0,0).
        gameState.playerLocation = static_cast<uint16_t>((4u << 8) | 12u);
        WorldEngine::init(worldState());
        memset(Arduboy2Base::sBuffer, 0xA5, 1024);
        drawMapFast(worldState());
        test.expectEq(pocketFramePixelsMatch(4), true,
                      F("deposit box renders the lure-box frame"));
    }
    plants = PlantGameState{};

    // The authored guide is visible on the floor cell and its fixed collision
    // guard keeps the player from walking through the sprite.
    test.expectEq(npcSpritesWidth, 16, F("npc width"));
    test.expectEq(npcSpritesHeight, 16, F("npc height"));
    test.expectEq(npcSpritesFrames, 1, F("one guide frame"));
    gameState.playerLocation = static_cast<uint16_t>((4u << 8) | 13u);
    WorldEngine::init(worldState());
    memset(Arduboy2Base::sBuffer, 0xA5, 1024);
    drawMapFast(worldState());
    test.expectEq(npcSpritePixelsMatch(), true,
                  F("guide NPC renders over its floor tile"));
    gameState.playerLocation = static_cast<uint16_t>((5u << 8) | 16u);
    WorldEngine::init(worldState());
    worldState().motion.directionAndFlags =
        static_cast<uint8_t>(Direction::UP);
    test.expectEq(WorldEngine::moveable(worldState()), false,
                  F("guide NPC blocks its authored cell"));

    // Frame-budget proof on the real world step: runMap + map/player/overlay
    // render + the real FX display, using the shared micros() instrumentation.
    // The deliberately slowed frame proves the assertion is non-vacuous.
    gameState.state = GameState_t::WORLD;
    gameState.playerLocation = static_cast<uint16_t>((7u << 8) | 12u);
    WorldEngine::init(worldState());
    WorldEngine::loadMap(worldState(), 0, 0);
    drawMapFast(worldState());
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

    // jp8.11.5 device path: the real A edge on the authored plot is driven from
    // the RAM property window. The full stage/full-bag/retry matrix is host-
    // proven; keep the empty-plant and mature-harvest device proof plus the
    // no-FX-read guarantee.
    {
        auto facePlant = []() {
            gameState.playerLocation = static_cast<uint16_t>((64u << 8) | 7u);
            WorldEngine::init(worldState());
            worldState().motion.directionAndFlags =
                static_cast<uint8_t>(Direction::RIGHT);
            TilePropertyWindow window(worldState().propertyWindow);
            window.begin(7, 63);
            uint16_t row[TilePropertyWindow::WINDOW_WIDTH] = {};
            row[1] = TileProps::packTile(9, TileProps::PROP_PLANTABLE);
            window.writeRow(1, row);
        };
        auto pressPlant = []() {
            arduboy.currentButtonState = A_BUTTON;
            arduboy.previousButtonState = 0;
            FxReadCounter::resetFrame();
            WorldEngine::interact();
            arduboy.currentButtonState = arduboy.previousButtonState = 0;
        };

        plants = PlantGameState{};
        item::inventoryClear(LureZone::inventory());
        facePlant();
        pressPlant();
        test.expectEq(plants.plantStages.getStage(FIRST_BETA_PLOT_INDEX), 1,
                      F("device A on an empty plot plants stage1"));
        test.expectEq(item::inventoryCount(LureZone::inventory(), item::ItemKind::Lure,
                                           item::lureId(0, 1)), 0,
                      F("device planting costs no lure"));
        test.expectEq(FxReadCounter::count(), static_cast<uint16_t>(0),
                      F("device plant press adds no FX read"));

        plants = PlantGameState{};
        for (uint8_t i = 0; i < 3; ++i)
            plants.plantStages.increment(FIRST_BETA_PLOT_INDEX);
        facePlant();
        pressPlant();
        test.expectEq(item::inventoryCount(LureZone::inventory(), item::ItemKind::Lure,
                                           item::lureId(0, 1)), 1,
                      F("device mature harvest grants one tier-1 lure"));
        test.expectEq(plants.plantStages.getStage(FIRST_BETA_PLOT_INDEX), 1,
                      F("device harvest resets the plot to planted"));
        test.expectEq(LureZone::messageAddr(), lureTextGathered,
                      F("device harvest shows the gathered feedback"));
    }

    // jp8.11.13 device path: the faced (21,9) coordinate from (20,9) starts the
    // one-member boss; once complete the same interaction shows the ending and
    // never restarts the fight. Host world/duel suites own the full flow.
    {
        auto faceBoss = []() {
            gameState.playerLocation = static_cast<uint16_t>((9u << 8) | 20u);
            WorldEngine::init(worldState());
            worldState().motion.directionAndFlags =
                static_cast<uint8_t>(Direction::RIGHT);
        };
        auto pressA = []() {
            arduboy.currentButtonState = A_BUTTON;
            arduboy.previousButtonState = 0;
            WorldEngine::interact();
            arduboy.currentButtonState = arduboy.previousButtonState = 0;
        };

        gameState.clearFlag(flag_beta_complete);
        player.basic();
        gameState.state = GameState_t::WORLD;
        faceBoss();
        pressA();
        test.expectEq(gameState.state, GameState_t::BATTLE,
                      F("device boss trigger starts the fight"));
        test.expectEq(battleSession().state().partyCount[1], 1,
                      F("device boss is one member"));
        test.expectEq(battleSession().state().active[1].id, 31,
                      F("device boss is ardu"));
        test.expectEq(battleSession().state().active[1].level, 8,
                      F("device boss is level 8"));
        exitBattle();
        gameState.state = GameState_t::WORLD;

        gameState.setFlag(flag_beta_complete);
        faceBoss();
        pressA();
        test.expectEq(gameState.state, GameState_t::WORLD,
                      F("device completed boss stays in world"));
        test.expectEq(LureZone::messageAddr(), lureTextBetaComplete,
                      F("device completed boss shows the ending"));
    }

    test.report(F("test_world_player"));
}
void loop() { exit(0); }
