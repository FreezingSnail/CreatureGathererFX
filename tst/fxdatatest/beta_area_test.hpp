#pragma once
// First-beta ordinary loop device proof (CreatureGathererFX-jp8.11.15).
//
// Complements tst/beta_area_flow_test.hpp. This sketch boots the shipping
// ordinary fresh game on the packed cart, walks the authored pocket through the
// real world loop and proves on hardware: the empty-deposit refusal, the
// authored plot growth/harvest, the deposit, a real step-triggered gatherable
// duel with the opponent's HP cost and the capture commit, the free rest, two
// explicit boss-win/ending v3 checkpoints with bag, plants, collection, party
// HP, experience and the completion flag together, and the fixed boss
// win/ending. The closing section proves the ordinary-win [[reward.heal]] grant
// and its real Heal-page spend on the packed tables. The 14-win XP/evolution
// ladder, the boss loss/retry, the world menu and the interrupted-save matrix
// stay in the host suite and the existing device suites (test_duel_session,
// test_collection, test_save).
//
// Wired by tst/fxdatatest/test_beta_area.ino.

#include <string.h>

#include <ArduboyFX.h>

#include "fxtest.hpp"
#include "src/GameState.hpp"
#include "src/creature/LevelCurve.hpp"
#include "src/engine/ModeState.hpp"
#include "src/engine/battle/BattleFlow.hpp"
#include "src/engine/battle/BattlePresets.hpp"
#include "src/engine/battle/BattleSession.hpp"
#include "src/engine/world/Encounter.hpp"
#include "src/engine/world/LureZone.hpp"
#include "src/engine/world/StepEvent.hpp"
#include "src/engine/world/TileProps.hpp"
#include "src/engine/world/TilePropertyWindow.hpp"
#include "src/engine/world/World.hpp"
#include "src/flags/flag_bit_array.hpp"
#include "src/fxdata.h"
#include "src/item/Inventory.hpp"
#include "src/item/ItemIds.hpp"
#include "src/lib/FxReadCounter.hpp"
#include "src/player/Player.hpp"
#include "src/plants/PlantGamestate.hpp"
#include "src/save/Compaction.hpp"
#include "src/save/FlashBackend.hpp"
#include "src/save/SaveController.hpp"
#include "src/save/SaveFile.hpp"

extern uint16_t flag_beta_complete;

namespace beta_area_fx_test_detail {

constexpr uint8_t START_X = 11;
constexpr uint8_t START_Y = 5;
constexpr uint8_t PLOT_X = 14;
constexpr uint8_t PLOT_Y = 2;
constexpr uint8_t BOX_X = 9;
constexpr uint8_t BOX_Y = 2;
constexpr uint8_t REST_X = 10;
constexpr uint8_t REST_Y = 4;
constexpr uint8_t BOSS_X = 21;
constexpr uint8_t BOSS_Y = 9;
constexpr uint8_t LURE_ITEM = 1;   // item::lureId(0, 1)
constexpr uint8_t LURE_ZONE = 4;   // LureZone::lureId(0, 1)
constexpr uint8_t NEED_LEVEL3 = 9;
constexpr uint8_t HERO_MAX = 39;
constexpr uint8_t GATHER_COST = 7;
constexpr uint16_t LEVEL8 = 512;

uint8_t lastInteractReads = 0;
uint8_t forcedGate = 0;
uint8_t gateHits = 0;
uint8_t tableSlot = 0;

uint8_t encounterRng(uint8_t lower, uint8_t upper) {
    (void)lower;
    if (upper == 99) {
        ++gateHits;
        return (forcedGate != 0 && gateHits == forcedGate) ? 0 : 99;
    }
    return tableSlot <= upper ? tableSlot : 0;
}

void setButtons(uint8_t current, uint8_t previous) {
    arduboy.currentButtonState = current;
    arduboy.previousButtonState = previous;
}

// Seed the authored pocket's walkability and faced-tile props. The packed map
// walk itself is proven by test_world_player; this suite keeps its flash budget
// for the loop, the real packed zone/creature data and the FX save flash.
void seedWindow() {
    const int16_t x = static_cast<uint8_t>(gameState.playerLocation);
    const int16_t y = static_cast<uint8_t>(gameState.playerLocation >> 8);
    uint16_t cells[TilePropertyWindow::WINDOW_HEIGHT]
                  [TilePropertyWindow::WINDOW_WIDTH];
    for (uint8_t row = 0; row < TilePropertyWindow::WINDOW_HEIGHT; ++row)
        for (uint8_t col = 0; col < TilePropertyWindow::WINDOW_WIDTH; ++col)
            cells[row][col] = TileProps::packTile(
                1, TileProps::PROP_WALKABLE | TileProps::PROP_ENCOUNTER);
    if (x == BOX_X && y == BOX_Y + 1)
        cells[1][3] = TileProps::packTile(9, TileProps::PROP_SPARE);
    if (x == PLOT_X && y == PLOT_Y + 1)
        cells[1][3] = TileProps::packTile(9, TileProps::PROP_PLANTABLE);
    TilePropertyWindow window(worldState().propertyWindow);
    window.begin(x - 3, y - 2);
    for (uint8_t row = 0; row < TilePropertyWindow::WINDOW_HEIGHT; ++row)
        window.writeRow(row, cells[row]);
}

bool worldFrame(uint8_t held, uint8_t edge) {
    setButtons(static_cast<uint8_t>(held | edge), held);
    WorldEngine::runMap(worldState());
    setButtons(0, 0);
    if (gameState.state == GameState_t::WORLD) seedWindow();
    return gameState.state == GameState_t::WORLD;
}

uint8_t directionButton(Direction direction) {
    switch (direction) {
    case Direction::LEFT: return LEFT_BUTTON;
    case Direction::RIGHT: return RIGHT_BUTTON;
    case Direction::UP: return UP_BUTTON;
    case Direction::DOWN: return DOWN_BUTTON;
    }
    return 0;
}

// Movement uses the real WorldEngine input/step seam. Arduboy2::pressed()
// samples the hardware pins, which a headless capture cannot drive, so the
// suite feeds the same buttons through inputFromButtons; moveChar/onStep are
// the shipping commit path (including the encounter roll).
bool walk(Direction direction, uint8_t tiles) {
    for (uint8_t tile = 0; tile < tiles; ++tile) {
        if (gameState.state != GameState_t::WORLD) return false;
        seedWindow();
        const uint16_t start = gameState.playerLocation;
        const uint8_t button = directionButton(direction);
        for (uint8_t frame = 0; frame < 16; ++frame) {
            WorldEngine::inputFromButtons(worldState(), button);
            WorldEngine::moveChar(worldState());
            if (gameState.state != GameState_t::WORLD) return false;
            seedWindow();
        }
        if (gameState.playerLocation == start) return false;
    }
    return true;
}

void face(Direction direction) {
    WorldEngine::inputFromButtons(worldState(), 0); // stop any held motion
    worldState().motion.directionAndFlags = static_cast<uint8_t>(
        (worldState().motion.directionAndFlags & ~0x03) |
        static_cast<uint8_t>(direction));
    seedWindow();
}

bool pressA() {
    FxReadCounter::resetFrame();
    setButtons(A_BUTTON, 0);
    WorldEngine::runMap(worldState());
    lastInteractReads = FxReadCounter::count();
    setButtons(0, 0);
    if (gameState.state == GameState_t::WORLD) seedWindow();
    return gameState.state == GameState_t::WORLD;
}

void menuEdge(uint8_t button) { worldFrame(0, button); }

bool battleFrame(uint8_t edge) {
    setButtons(edge, 0);
    BattleFlow::update(edge);
    setButtons(0, 0);
    return gameState.state == GameState_t::BATTLE;
}

void drainBattle() {
    for (uint16_t frame = 0;
         frame < 4096 && gameState.state == GameState_t::BATTLE; ++frame) {
        if (battleSession().awaitingPlayer()) return;
        battleFrame(0);
    }
}

void gatherRound() {
    battleFrame(0);
    battleFrame(MENU_EDGE_B);
    battleFrame(MENU_EDGE_A);
    drainBattle();
}

void attackRound() {
    battleFrame(0);
    battleFrame(MENU_EDGE_A);
    drainBattle();
}

void eraseSaves() {
    const uint16_t pages[] = {32, 48};
    for (uint8_t i = 0; i < 2; ++i) {
        flash.eraseSector(pages[i]);
        while (flash.busy()) {}
    }
}

bool runSave() {
    SaveController::begin(GameState_t::WORLD);
    for (uint16_t frame = 0; frame < 4096 && gameState.state == GameState_t::SAVING;
         ++frame) {
        SaveController::advance();
        // The shipping loop runs one advance per 20 ms frame; a tight test loop
        // must wait out each erase/program before the next step inspects flash.
        FX::waitWhileBusy();
    }
    return gameState.state == GameState_t::WORLD;
}

void freshGame() {
    eraseSaves();
    storeFresh();
    player = Player();
    plants = PlantGameState{};
    gameState.playerLocation = 0;
    memset(gameState.flags, 0, sizeof(FLAG_BIT_ARRAY));
    gameState.state = GameState_t::WORLD;
    menu.clear();
    dialogMenu.clear();
    LureZone::clear();
    LureZone::closePicker();
    forcedGate = 0;
    gateHits = 0;
    tableSlot = 0;
    StepEventTest::setEncounterRng(encounterRng);

    // The suite erases both v3 sectors, so this is the ordinary no-save boot.
    const bool restored = false;
    gameState.playerLocation =
        static_cast<uint16_t>((START_Y << 8) | START_X);
    enterBattle();
    exitBattle();
    gameState.state = GameState_t::WORLD;
    if (!restored) {
        battle::applyPlayerPreset(player, BattlePresets::beta);
        item::setBattleInventory(&player.inventory);
        SaveController::seedParty();
    }
    // The first walk seeds the authored property window before its first step.
}

} // namespace beta_area_fx_test_detail

inline void test_beta_area(FxTest &test) {
    using namespace beta_area_fx_test_detail;
    freshGame();

    // --- ordinary fresh boot ------------------------------------------------
    test.expectEq(gameState.playerLocation,
                  static_cast<uint16_t>((START_Y << 8) | START_X),
                  F("start"));
    test.expectEq(player.party[0].id, 0, F("species"));
    test.expectEq(player.party[0].level, 3, F("level"));
    test.expectEq(player.creatureHPs[0], player.party[0].hp, F("hp"));
    test.expectEq(player.party[2].id, 6, F("species"));
    test.expectEq(item::inventoryNonZeroCount(player.inventory), 0, F("bag"));
    test.expectEq(gameState.getFlag(flag_beta_complete), 0, F("flag"));

    // --- empty deposit refusal ----------------------------------------------
    test.expectEq(walk(Direction::LEFT, 2), true, F("walk"));
    test.expectEq(walk(Direction::UP, 2), true, F("walk"));
    face(Direction::UP);
    test.expectEq(pressA(), true, F("world"));
    test.expectEq(LureZone::pickerOpen(), 1, F("picker"));
    menuEdge(A_BUTTON);
    test.expectEq(LureZone::pickerFeedback(), 0, F("refuse"));
    test.expectEq(LureZone::active(), 0, F("inert"));
    menuEdge(B_BUTTON);

    // --- plant growth and harvest -------------------------------------------
    test.expectEq(walk(Direction::RIGHT, 5), true, F("walk"));
    face(Direction::UP);
    test.expectEq(pressA(), true, F("world"));
    test.expectEq(plants.plantStages.getStage(FIRST_BETA_PLOT_INDEX), 1, F("plant"));
    test.expectEq(lastInteractReads, 0, F("no read"));
    for (uint16_t tick = 0; tick < PlantGameState::GROWTH_STEPS * 2; ++tick) plants.tick();
    test.expectEq(plants.plantStages.getStage(FIRST_BETA_PLOT_INDEX), 3, F("mature"));
    face(Direction::UP);
    test.expectEq(pressA(), true, F("world"));
    test.expectEq(item::inventoryCount(player.inventory, item::ItemKind::Lure,
                                       LURE_ITEM), 1, F("harvest"));
    test.expectEq(plants.plantStages.getStage(FIRST_BETA_PLOT_INDEX), 1, F("reset"));

    // --- deposit ------------------------------------------------------------
    test.expectEq(walk(Direction::LEFT, 5), true, F("walk"));
    face(Direction::UP);
    test.expectEq(pressA(), true, F("world"));
    menuEdge(A_BUTTON);
    test.expectEq(LureZone::id(), LURE_ZONE, F("zone"));
    test.expectEq(LureZone::charge(), 32, F("charge"));
    test.expectEq(item::inventoryCount(player.inventory, item::ItemKind::Lure,
                                       LURE_ITEM), 0, F("spend"));
    menuEdge(B_BUTTON);

    // --- gatherable encounter, opponent HP cost, capture --------------------
    gateHits = 0;
    forcedGate = 1;
    tableSlot = 0;
    test.expectEq(walk(Direction::DOWN, 1), false, F("encounter"));
    test.expectEq(gameState.state, GameState_t::BATTLE, F("battle"));
    const battle::BattleState &wild = battleSession().state();
    test.expectEq(wild.gatherable, 1, F("gather"));
    test.expectEq(wild.active[1].id, 9, F("species"));
    test.expectEq(wild.gather.need, NEED_LEVEL3, F("need"));
    test.expectEq(wild.gather.tierRate, 2, F("rate"));
    test.expectEq(wild.active[0].maxHp, HERO_MAX, F("hero hp"));
    gatherRound();
    gatherRound();
    test.expectEq(battleSession().state().active[0].hp,
                  static_cast<uint8_t>(HERO_MAX - GATHER_COST * 2), F("cost"));
    gatherRound();
    gatherRound();
    gatherRound();
    test.expectEq(gameState.state, GameState_t::WORLD, F("world"));
    test.expectEq(player.creatureHPs[0], HERO_MAX - GATHER_COST * 5, F("cost"));
    StoreRecord captured = {};
    test.expectEq(storeRead(3, captured), 1, F("slot"));
    test.expectEq(captured.id, 9, F("species"));
    test.expectEq(storeInstanceHP(captured), 45, F("hp"));

    // --- free rest ----------------------------------------------------------
    face(Direction::RIGHT);
    test.expectEq(pressA(), true, F("world"));
    test.expectEq(player.creatureHPs[0], player.party[0].hp, F("rest"));

    // The exact 14-win XP/evolution ladder is proven by the host suite; the
    // device checkpoint keeps the authored level-3 party and adds one authored
    // lure (the harvest/deposit mechanics are proven above) for the bag field.
    item::inventoryAdd(player.inventory, item::ItemKind::Lure, LURE_ITEM, 1);
    test.expectEq(item::inventoryCount(player.inventory, item::ItemKind::Lure,
                                       LURE_ITEM), 1, F("bag"));

    // --- fixed boss: the persistent ending ----------------------------------
    gameState.playerLocation = static_cast<uint16_t>((9 << 8) | 20);
    WorldEngine::init(worldState());
    WorldEngine::loadMap(worldState(), 0, 0);
    seedWindow();
    face(Direction::RIGHT);
    test.expectEq(pressA(), false, F("boss"));
    test.expectEq(battleSession().state().trainer, 1, F("trainer"));
    test.expectEq(battleSession().state().active[1].id, 31, F("boss id"));
    test.expectEq(battleSession().state().active[1].level, 8, F("boss lv"));
    {
        battle::BattleState &state =
            const_cast<battle::BattleState &>(battleSession().state());
        state.active[1].hp = 1;
    }
    attackRound();
    drainBattle();
    test.expectEq(gameState.state, GameState_t::WORLD, F("win"));
    test.expectEq(gameState.getFlag(flag_beta_complete), 1, F("flag"));
    // Boss-win checkpoint: a first explicit save from the post-boss world.
    test.expectEq(runSave(), true, F("save"));
    SaveFile bossSnap = {};
    test.expectEq(saveFileLoad(bossSnap), true, F("reload"));
    test.expectEq(static_cast<uint8_t>(bossSnap.flags[0] & 0x02), 2, F("flag"));
    face(Direction::RIGHT);
    test.expectEq(pressA(), true, F("world"));
    test.expectEq(LureZone::messageAddr(), lureTextBetaComplete, F("ending"));

    // --- the ending and the loop fields persist together --------------------
    const uint8_t endingHP = player.creatureHPs[0];
    const uint16_t endingLocation = gameState.playerLocation;
    test.expectEq(runSave(), true, F("save"));
    SaveFile snap = {};
    test.expectEq(saveFileLoad(snap), true, F("reload"));
    test.expectEq(static_cast<uint8_t>(snap.flags[0] & 0x02), 2, F("flag"));
    test.expectEq(snap.lures[LURE_ITEM], 1, F("bag"));
    test.expectEq(static_cast<uint8_t>(snap.plantStages[0] & 0x03), 1, F("plant"));
    test.expectEq(snap.partyHP[0], endingHP, F("hp"));
    test.expectEq(snap.playerLocation, endingLocation, F("loc"));
    test.expectEq(endingHP < player.party[0].hp, 1, F("hurt"));
    StoreRecord alive = {};
    test.expectEq(storeRead(0, alive), 1, F("record"));
    test.expectEq(levelFromExp(alive.exp), 3, F("level"));
    test.expectEq(storeRead(3, alive), 1, F("capture"));

    // --- ordinary wild win banks and spends the authored heal ---------------
    // The boss above is a trainer win and granted none; this ordinary
    // non-gatherable KO runs the real terminal edge and grants both authored
    // [[reward.heal]] entries through the live bag. A later duel then spends the
    // first one from the real Heal page.
    item::inventoryClear(player.inventory);
    enterBattle();
    gameState.state = GameState_t::BATTLE;
    battleSession().beginWild(9, 3, false, 0);
    {
        battle::BattleState &state =
            const_cast<battle::BattleState &>(battleSession().state());
        state.active[1].hp = 1;
    }
    attackRound();
    test.expectEq(gameState.state, GameState_t::WORLD, F("wild win"));
    test.expectEq(item::inventoryCount(player.inventory,
                                       item::ItemKind::Consumable, 0), 1,
                  F("heal 0"));
    test.expectEq(item::inventoryCount(player.inventory,
                                       item::ItemKind::Consumable, 1), 1,
                  F("heal 1"));

    enterBattle();
    gameState.state = GameState_t::BATTLE;
    battleSession().beginWild(9, 3, false, 0);
    uint8_t duelMax = 0;
    {
        battle::BattleState &state =
            const_cast<battle::BattleState &>(battleSession().state());
        duelMax = state.active[0].maxHp;
        state.active[0].hp = static_cast<uint8_t>(duelMax - 20);
    }
    battleFrame(0);           // choice entry -> pick row
    battleFrame(MENU_EDGE_B); // non-gatherable wild: ITEMS first
    battleFrame(MENU_EDGE_A); // open the Heal page
    battleFrame(MENU_EDGE_A); // confirm the authored heal
    test.expectEq(item::inventoryCount(player.inventory,
                                       item::ItemKind::Consumable, 0), 0,
                  F("take"));
    drainBattle();
    test.expectEq(battleSession().state().active[0].hp,
                  static_cast<uint8_t>(duelMax - 20 + 16 -
                                       battleSession().state().damage[0]),
                  F("heal"));
    test.expectEq(item::inventoryCount(player.inventory,
                                       item::ItemKind::Consumable, 0), 0,
                  F("spent"));

    test.report(F("test_beta_area"));
}
