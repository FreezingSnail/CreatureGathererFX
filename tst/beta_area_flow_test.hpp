#pragma once
// First-beta ordinary loop integration proof (CreatureGathererFX-jp8.11.15).
//
// One continuous scenario boots the shipping ordinary fresh game (duel +
// compact warp VM profile: no demo flags, full-VM override, temporary bag or
// debug grants), then drives fresh world/picker/battle/menu button edges
// through the real WorldEngine::runMap, LureZone picker, BattleFlow::update,
// CollectionMenu world menu and SaveController state machines. Component
// matrices stay in their own suites; this suite proves the loop connects them
// and that the v3 checkpoint publishes bag, plants, collection, party HP,
// experience and the completion flag together.
//
// Native limitation: the host FX fake has no authored 128 KiB map, so the
// property window is seeded per frame with the authored pocket's walkability
// (open floor walkable, the plot/box faced tiles occupied but not walkable).
// The device suite (beta_area_test.hpp) proves the real cart data/route state
// machine and FX save flash; the authored map walk itself is covered by the
// existing test_world_player/jp8.11.25 map suites.

#include <string.h>

#include "test.hpp"
#include "host/Arduboy2.h"
#include "src/FXDataFake.hpp"
#include "src/WorldInteractionFake.hpp"

#include "../src/creature/LevelCurve.hpp"
#include "../src/engine/ModeState.hpp"
#include "../src/engine/battle/BattleFlow.hpp"
#include "../src/engine/battle/BattlePresets.hpp"
#include "../src/engine/battle/BattleSession.hpp"
#include "../src/engine/battle/DuelKernel.hpp"
#include "../src/engine/menu/CollectionMenu.hpp"
#include "../src/engine/menu/InventoryMenu.hpp"
#include "../src/engine/world/Encounter.hpp"
#include "../src/engine/world/LureZone.hpp"
#include "../src/engine/world/StepEvent.hpp"
#include "../src/engine/world/TileProps.hpp"
#include "../src/engine/world/TilePropertyWindow.hpp"
#include "../src/engine/world/World.hpp"
#include "../src/flags/flag_bit_array.hpp"
#include "../src/globals.hpp"
#include "../src/fxdata.h"
#include "../src/item/Inventory.hpp"
#include "../src/item/ItemIds.hpp"
#include "../src/player/Player.hpp"
#include "../src/save/Compaction.hpp"
#include "../src/save/FlashBackend.hpp"
#include "../src/save/Journal.hpp"
#include "../src/save/SaveController.hpp"
#include "../src/save/SaveFile.hpp"
#include "fxdatatest/generated/reward_data.hpp"

// Defined once by World.cpp's generated-flags mirror include.
extern uint16_t flag_beta_complete;

// The ordinary-win heal grant (BattleFlow.cpp) bounds its loop with the packed
// span between the two generated heal columns; the packer writes them back to
// back. Pin both the authored two-entry count and that span to the generated
// fixture so an authored table or packer-layout drift fails the host gate.
static_assert(RewardFixture::healRewardCount == 2,
              "the authored first-beta heal reward table keeps two entries");
static_assert(static_cast<uint16_t>(Rewards::healCount) -
                      static_cast<uint16_t>(Rewards::healItem) ==
                  RewardFixture::healRewardCount,
              "runtime heal reward count must match the generated fixture");

namespace worldMovementFake {
extern uint8_t pendingButtons;
void reset();
void requestButtons(uint8_t buttons);
uint16_t inputCalls();
}

namespace beta_flow_test_detail {

// Authored landmarks (data/beta-area.toml). The runtime compares packed
// coordinates; the native walk uses the same coordinates on a seeded window.
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
constexpr uint16_t START_TILE = static_cast<uint16_t>((START_Y << 8) | START_X);
constexpr uint8_t LURE_TIER1_TYPE0 = 1; // item::lureId(0, 1), [plant_source]
constexpr uint8_t NEED_LEVEL3 = 9;      // clamp(8 + 3/2, 8, 24)
constexpr uint8_t DUEL_HALF_WIN = 7;    // DMG_WIN 14 scaled x0.5
constexpr uint8_t DUEL_HALF_CHIP = 2;   // DMG_CHIP 4 scaled x0.5
constexpr uint16_t EVOLVE_EXP = 512;    // LEVEL_EXP[8]
constexpr uint8_t HEAL_ARG0 = 16;       // data/consumables.csv row 0: Heal 16

// Authored packed encounter block, mirrored from fxdata/data/encounters.txt so
// the native zone load/select path runs without a host filesystem. The device
// suite proves the real packed image; this fixture is only the native mirror.
const uint8_t kZoneDefs[] = {
    1, 0, 0, 5, 15, 1, 2, 3, 4, 0,  2, 0, 0, 5, 15, 1, 2, 3, 4, 0,
    33, 0, 0, 5, 15, 1, 2, 3, 4, 0, 34, 0, 0, 5, 15, 1, 2, 3, 4, 0,
    65, 0, 0, 5, 15, 1, 2, 3, 4, 0, 66, 0, 0, 5, 15, 1, 2, 3, 4, 0,
    255, 255, 0, 0, 0, 0, 0, 0, 0, 0, 255, 255, 0, 0, 0, 0, 0, 0, 0, 0};
const uint8_t kTables[] = {
    0, 3, 6, 9, 10, 12, 13, 14, 0, 6, 2, 5,  9, 9, 0, 6, 9, 13, 3, 9, 14,
    9, 2, 5, 6, 6, 3, 6, 9, 6, 14, 6, 0, 6, 2, 5, 0, 13, 0, 13, 9, 0,
    13, 6, 0, 13, 2, 5, 3, 14, 3, 14, 9, 3, 14, 6, 3, 14, 2, 5};

struct Hud {
    uint16_t worldFrames = 0;
    uint16_t battleFrames = 0;
    uint16_t steps = 0;
    uint8_t encounters = 0;
    uint8_t wins = 0;
    uint8_t captures = 0;
    uint8_t bossAttempts = 0;
};

Hud hud;
uint16_t gateCount = 0;
uint8_t forcedGateAt = 0;
uint8_t forcedSlot = 0;

uint8_t flowRng(uint8_t lower, uint8_t upper) {
    (void)lower;
    if (upper == 99) {
        ++gateCount;
        return (forcedGateAt != 0 && gateCount == forcedGateAt) ? 0 : 99;
    }
    return forcedSlot <= upper ? forcedSlot : 0;
}

void installZoneFixture() {
    fxDataFake::dataBase = EncounterFXData::zoneDefs;
    memset(fxDataFake::dataBytes, 0, sizeof(fxDataFake::dataBytes));
    memcpy(fxDataFake::dataBytes, kZoneDefs, sizeof(kZoneDefs));
    memcpy(fxDataFake::dataBytes + (EncounterFXData::tables - EncounterFXData::zoneDefs),
           kTables, sizeof(kTables));
}

void installEvolutionFixture() {
    const uint32_t targetOffset =
        static_cast<uint32_t>(Evolution::evolutionTarget - Evolution::evolutionSource);
    const uint32_t levelOffset =
        static_cast<uint32_t>(Evolution::evolutionLevel - Evolution::evolutionSource);
    fxDataFake::dataBase = Evolution::evolutionSource;
    memset(fxDataFake::dataBytes, 0, sizeof(fxDataFake::dataBytes));
    const uint8_t sources[6] = {0, 1, 3, 4, 6, 7};
    const uint8_t targets[6] = {1, 2, 4, 5, 7, 8};
    const uint8_t levels[6] = {8, 16, 8, 16, 8, 16};
    memcpy(fxDataFake::dataBytes, sources, sizeof(sources));
    memcpy(fxDataFake::dataBytes + targetOffset, targets, sizeof(targets));
    memcpy(fxDataFake::dataBytes + levelOffset, levels, sizeof(levels));

    // The same ladder also proves the ordinary-win heal grant, so the packed
    // reward columns are mirrored from the generated fixture inside the one
    // window anchored at the evolution source.
    const uint32_t itemOffset = static_cast<uint32_t>(
        Rewards::healItem - Evolution::evolutionSource);
    const uint32_t countOffset = static_cast<uint32_t>(
        Rewards::healCount - Evolution::evolutionSource);
    static_assert(Rewards::healItem > Evolution::evolutionSource,
                  "the heal columns must follow the evolution anchor");
    static_assert(static_cast<uint32_t>(Rewards::healCount -
                                        Evolution::evolutionSource) +
                          RewardFixture::healRewardCount <=
                      sizeof(fxDataFake::dataBytes),
                  "the packed heal columns must fit the native FX window");
    for (uint8_t i = 0; i < RewardFixture::healRewardCount; ++i) {
        fxDataFake::dataBytes[itemOffset + i] = RewardFixture::healItem[i];
        fxDataFake::dataBytes[countOffset + i] = RewardFixture::healCount[i];
    }
}

// Host fixture for the packed consumable records (data/consumables.csv rows 0
// and 1, the two authored Heal drops): Heal 16 and Heal 32. The device suite
// proves the real packed table; this is only the native mirror so the item-use
// round can read a definition without a cart image.
void installConsumableFixture() {
    fxDataFake::dataBase = consumable_table;
    memset(fxDataFake::dataBytes, 0, sizeof(fxDataFake::dataBytes));
    const uint8_t rows[2][2] = {{1, 16}, {1, 32}};
    memcpy(fxDataFake::dataBytes, rows, sizeof(rows));
}

uint8_t moveButton(Direction direction) {
    switch (direction) {
    case Direction::LEFT: return LEFT_BUTTON;
    case Direction::RIGHT: return RIGHT_BUTTON;
    case Direction::UP: return UP_BUTTON;
    case Direction::DOWN: return DOWN_BUTTON;
    }
    return 0;
}

uint8_t facing() {
    return static_cast<uint8_t>(worldState().motion.directionAndFlags & 0x03);
}

void seedWindow(int16_t blockX, int16_t blockY, uint8_t blockProps) {
    const int16_t x = static_cast<uint8_t>(gameState.playerLocation);
    const int16_t y = static_cast<uint8_t>(gameState.playerLocation >> 8);
    uint16_t cells[TilePropertyWindow::WINDOW_HEIGHT]
                  [TilePropertyWindow::WINDOW_WIDTH];
    for (uint8_t row = 0; row < TilePropertyWindow::WINDOW_HEIGHT; ++row)
        for (uint8_t col = 0; col < TilePropertyWindow::WINDOW_WIDTH; ++col)
            cells[row][col] = TileProps::packTile(
                1, TileProps::PROP_WALKABLE | TileProps::PROP_ENCOUNTER);
    if (blockX >= 0) {
        const int16_t col = blockX - (x - 3);
        const int16_t row = blockY - (y - 2);
        if (col >= 0 && col < TilePropertyWindow::WINDOW_WIDTH && row >= 0 &&
            row < TilePropertyWindow::WINDOW_HEIGHT)
            cells[row][col] = TileProps::packTile(9, blockProps);
    }
    TilePropertyWindow window(worldState().propertyWindow);
    window.begin(x - 3, y - 2);
    for (uint8_t row = 0; row < TilePropertyWindow::WINDOW_HEIGHT; ++row)
        window.writeRow(row, cells[row]);
}

// One ordinary world frame: the render path would have seeded the window from
// the packed map on the previous frame; the native fixture refreshes it here.
bool worldFrame(uint8_t moveButtons = 0, bool pressA = false, int16_t blockX = -1,
                int16_t blockY = -1, uint8_t blockProps = 0) {
    seedWindow(blockX, blockY, blockProps);
    worldInteractionFake::pickerButtons = 0;
    worldMovementFake::pendingButtons = moveButtons;
    worldInteractionFake::pressedA = pressA;
    WorldEngine::runMap(worldState());
    worldMovementFake::pendingButtons = 0;
    worldInteractionFake::pressedA = false;
    ++hud.worldFrames;
    return gameState.state == GameState_t::WORLD;
}

// Walk one tile with a held direction. A committed destination that starts a
// battle returns false with the battle state active and the location committed.
bool walkTile(Direction direction) {
    if (gameState.state != GameState_t::WORLD) return false;
    const uint16_t start = gameState.playerLocation;
    worldMovementFake::pendingButtons = moveButton(direction);
    for (uint8_t frame = 0; frame < 18; ++frame) {
        seedWindow(-1, -1, 0);
        worldInteractionFake::pickerButtons = 0;
        worldInteractionFake::pressedA = false;
        WorldEngine::runMap(worldState());
        ++hud.worldFrames;
        if (gameState.state != GameState_t::WORLD) {
            worldMovementFake::pendingButtons = 0;
            return false;
        }
    }
    worldMovementFake::pendingButtons = 0;
    if (gameState.playerLocation == start) return false;
    ++hud.steps;
    return true;
}

bool walk(Direction direction, uint8_t tiles) {
    for (uint8_t tile = 0; tile < tiles; ++tile)
        if (!walkTile(direction)) return false;
    return true;
}

// Turn toward a faced tile without entering it (the authored landmark tiles are
// occupied and not walkable).
void face(Direction direction, int16_t targetX, int16_t targetY,
          uint8_t props) {
    worldMovementFake::pendingButtons = moveButton(direction);
    seedWindow(targetX, targetY, props);
    worldInteractionFake::pickerButtons = 0;
    worldInteractionFake::pressedA = false;
    WorldEngine::runMap(worldState());
    worldMovementFake::pendingButtons = 0;
    ++hud.worldFrames;
}

bool pressA(int16_t targetX, int16_t targetY, uint8_t props) {
    seedWindow(targetX, targetY, props);
    worldInteractionFake::pickerButtons = 0;
    worldInteractionFake::pressedA = true;
    WorldEngine::runMap(worldState());
    worldInteractionFake::pressedA = false;
    ++hud.worldFrames;
    return gameState.state == GameState_t::WORLD;
}

void menuFrame(uint8_t edges) {
    seedWindow(-1, -1, 0);
    worldInteractionFake::pickerButtons = edges;
    worldInteractionFake::pressedA = false;
    WorldEngine::runMap(worldState());
    worldInteractionFake::pickerButtons = 0;
    ++hud.worldFrames;
}

// --- battle frames ---------------------------------------------------------
void drainFrames(uint16_t limit) {
    for (uint16_t frame = 0; frame < limit && gameState.state == GameState_t::BATTLE;
         ++frame) {
        BattleFlow::update(0);
        ++hud.battleFrames;
    }
}

void edge(uint8_t buttons) {
    if (gameState.state != GameState_t::BATTLE) return;
    BattleFlow::update(buttons);
    ++hud.battleFrames;
}

// Finish the current round or terminal after a submitted intent.
void drainToChoiceOrWorld() {
    for (uint16_t frame = 0;
         frame < 4096 && gameState.state == GameState_t::BATTLE; ++frame) {
        if (battleSession().awaitingPlayer()) return;
        BattleFlow::update(0);
        ++hud.battleFrames;
    }
}

// Choice frame, action page (B), GATHER (A), then the atomic round.
void gatherRound() {
    edge(0);               // choice entry builds the pick row
    edge(MENU_EDGE_B);     // wild action page: GATHER, ITEMS, ESCAPE
    edge(MENU_EDGE_A);     // cursor 0 is GATHER
    drainToChoiceOrWorld();
}

void attackRound() {
    edge(0);               // choice entry builds the pick row
    edge(MENU_EDGE_A);     // cursor 0 is the lowest set attack bit
    drainToChoiceOrWorld();
}

// Leave a non-gatherable wild through the real ESCAPE row (it never grants a
// reward), so a proof duel can end without a KO.
void escapeRound() {
    edge(0);               // choice entry builds the pick row
    edge(MENU_EDGE_B);     // action page
    edge(static_cast<uint8_t>(MENU_NAV_DOWN)); // ITEMS -> ESCAPE
    edge(MENU_EDGE_A);     // ESCAPE resolves immediately
    drainFrames(4096);
}

uint8_t flexibleBattleRng(uint8_t bound) { return bound == 0 ? 0 : bound - 1; }

void ordinaryWin(uint8_t foeSpecies, uint8_t foeLevel) {
    enterBattle();
    gameState.state = GameState_t::BATTLE;
    battleSession().setRng({flexibleBattleRng});
    battleSession().beginWild(foeSpecies, foeLevel, false, 0);
    for (uint16_t guard = 0; guard < 64 && gameState.state == GameState_t::BATTLE;
         ++guard) {
        if (!battleSession().awaitingPlayer()) {
            if (!battleSession().isActive()) break;
            drainToChoiceOrWorld();
            continue;
        }
        attackRound();
    }
    drainFrames(4096);
    if (gameState.state == GameState_t::WORLD) ++hud.wins;
}

// --- save helpers ----------------------------------------------------------
// SaveController owns a private compaction buffer; drive it through its public
// advance() state machine instead of reaching into the snapshot.
SaveStep runControllerSave(uint16_t &frames) {
    SaveController::begin(GameState_t::WORLD);
    for (frames = 0; frames < 1024 && gameState.state == GameState_t::SAVING;
         ++frames)
        SaveController::advance();
    return gameState.state == GameState_t::SAVING ? SaveStep::Failed : SaveStep::Done;
}

void freshGame() {
    flashFakeReset();
    storeFresh();
    player = Player();
    plants = PlantGameState{};
    gameState.state = GameState_t::WORLD;
    gameState.playerLocation = START_TILE;
    gameState.flags[0] = 0;
    memset(gameState.flags, 0, sizeof(FLAG_BIT_ARRAY));
    menu.clear();
    dialogMenu.clear();
    worldInteractionFake::reset();
    worldMovementFake::reset();
    LureZone::clear();
    LureZone::closePicker();
    LureZone::dismissIntro();
    installZoneFixture();
    StepEventTest::setEncounterRng(flowRng);
    gateCount = 0;
    forcedGateAt = 0;
    forcedSlot = 0;
    hud = Hud{};

    // Ordinary fresh start, mirroring CreatureGathererFX.ino's shipping branch:
    // enter/exit initializes the world at the authored start and loads the
    // zone; the authored level-3 beta preset replaces the legacy party.
    enterBattle();
    exitBattle();
    gameState.state = GameState_t::WORLD;
    battle::applyPlayerPreset(player, BattlePresets::beta);
    item::setBattleInventory(&player.inventory);
    SaveController::seedParty();
    // The shipping loop renders before the first input frame; seed the window.
    seedWindow(-1, -1, 0);
}

uint8_t occupiedSlots() {
    uint8_t count = 0;
    for (uint8_t slot = 0; slot < STORE_CAPACITY; ++slot) {
        StoreRecord record;
        if (storeRead(slot, record) && record.id != STORE_RECORD_TOMBSTONE_ID) ++count;
    }
    return count;
}

} // namespace beta_flow_test_detail

inline void FirstBetaAreaFlowSuite(TestRunner &runner) {
    using namespace beta_flow_test_detail;
    TestSuite suite("First-beta ordinary area loop");

    Test flow("Ordinary fresh start, plant, deposit, gather and recovery");
    freshGame();

    // --- ordinary fresh setup ------------------------------------------------
    flow.assert(gameState.playerLocation, START_TILE,
                "fresh game starts at the authored pocket start");
    flow.assert(static_cast<uint8_t>(player.party[0].id), static_cast<uint8_t>(0),
                "fresh slot 0 is authored species 0");
    flow.assert(static_cast<uint8_t>(player.party[1].id), static_cast<uint8_t>(3),
                "fresh slot 1 is authored species 3");
    flow.assert(static_cast<uint8_t>(player.party[2].id), static_cast<uint8_t>(6),
                "fresh slot 2 is authored species 6");
    for (uint8_t i = 0; i < 3; ++i) {
        flow.assert(player.party[i].level, static_cast<uint8_t>(3),
                    "fresh party is level 3");
        flow.assert(player.creatureHPs[i], player.party[i].hp,
                    "fresh party is at full HP");
        flow.assert(storePartySlot(i), i, "fresh party references its seeded instance");
    }
    flow.assert(item::inventoryNonZeroCount(player.inventory),
                static_cast<uint8_t>(0), "fresh bag is empty");
    flow.assert(plants.plantStages.getStage(FIRST_BETA_PLOT_INDEX),
                static_cast<uint8_t>(0), "fresh plot is empty");
    flow.assert(gameState.getFlag(flag_beta_complete), false,
                "fresh game has no completion flag");

    // --- rejected deposit on an empty bag -----------------------------------
    flow.assert(walk(Direction::LEFT, 2), true, "walk left to the box column");
    flow.assert(walk(Direction::UP, 2), true, "walk up to the box approach");
    face(Direction::UP, BOX_X, BOX_Y, TileProps::PROP_SPARE);
    const uint8_t gatesBeforeBox = gateCount;
    pressA(BOX_X, BOX_Y, TileProps::PROP_SPARE);
    flow.assert(LureZone::pickerOpen(), true, "A on the box opens the material picker");
    menuFrame(LureZone::PICKER_BTN_A);
    flow.assert(LureZone::pickerFeedback(), false, "an empty bag refuses the deposit");
    flow.assert(LureZone::active(), false, "a refused deposit leaves the zone inert");
    flow.assert(gateCount, gatesBeforeBox, "world interaction starts no encounter roll");
    menuFrame(LureZone::PICKER_BTN_B);
    flow.assert(LureZone::pickerOpen(), false, "B closes the empty picker");

    // --- plant growth and harvest -------------------------------------------
    flow.assert(walk(Direction::RIGHT, 5), true, "walk right to the plot approach");
    face(Direction::UP, PLOT_X, PLOT_Y, TileProps::PROP_PLANTABLE);
    pressA(PLOT_X, PLOT_Y, TileProps::PROP_PLANTABLE);
    flow.assert(plants.plantStages.getStage(FIRST_BETA_PLOT_INDEX),
                static_cast<uint8_t>(1), "A on the empty plot plants it");
    flow.assert(item::inventoryCount(player.inventory, item::ItemKind::Lure,
                                     LURE_TIER1_TYPE0),
                static_cast<uint8_t>(0), "planting spends no lure");

    const uint16_t plantStepBase = hud.steps;
    constexpr uint8_t growthSteps = PlantGameState::GROWTH_STEPS;
    for (uint8_t i = 0; i < growthSteps; ++i) {
        if (!walk((i & 1) ? Direction::RIGHT : Direction::LEFT, 1)) break;
    }
    flow.assert(hud.steps - plantStepBase, static_cast<uint16_t>(growthSteps),
                "the plot advances after the configured growth steps");
    flow.assert(plants.plantStages.getStage(FIRST_BETA_PLOT_INDEX),
                static_cast<uint8_t>(2), "the planted plot is growing");

    for (uint8_t i = 0; i < growthSteps; ++i) {
        if (!walk((i & 1) ? Direction::RIGHT : Direction::LEFT, 1)) break;
    }
    flow.assert(plants.plantStages.getStage(FIRST_BETA_PLOT_INDEX),
                static_cast<uint8_t>(3), "the plot matures after two growth windows");
    face(Direction::UP, PLOT_X, PLOT_Y, TileProps::PROP_PLANTABLE);
    pressA(PLOT_X, PLOT_Y, TileProps::PROP_PLANTABLE);
    flow.assert(item::inventoryCount(player.inventory, item::ItemKind::Lure,
                                     LURE_TIER1_TYPE0),
                static_cast<uint8_t>(1), "a mature harvest yields one tier-1 lure");
    flow.assert(plants.plantStages.getStage(FIRST_BETA_PLOT_INDEX),
                static_cast<uint8_t>(1), "harvest resets the plot to planted");
    flow.assert(LureZone::messageAddr(), lureTextGathered,
                "the harvest shows its world message");

    // --- deposit ------------------------------------------------------------
    flow.assert(walk(Direction::LEFT, 5), true, "walk back to the box column");
    face(Direction::UP, BOX_X, BOX_Y, TileProps::PROP_SPARE);
    pressA(BOX_X, BOX_Y, TileProps::PROP_SPARE);
    flow.assert(LureZone::pickerOpen(), true, "A reopens the picker with the harvest");
    flow.assert(LureZone::pickerCount(), static_cast<uint8_t>(1),
                "the picker lists the harvested lure");
    menuFrame(LureZone::PICKER_BTN_A);
    flow.assert(LureZone::pickerFeedback(), true, "A spends the listed lure");
    flow.assert(item::inventoryCount(player.inventory, item::ItemKind::Lure,
                                     LURE_TIER1_TYPE0),
                static_cast<uint8_t>(0), "the deposit consumes exactly one lure");
    flow.assert(LureZone::active(), true, "the deposit activates the zone");
    flow.assert(LureZone::id(), static_cast<uint8_t>(4),
                "the zone encodes type 0 / tier 1 as id 4");
    flow.assert(LureZone::charge(), static_cast<uint8_t>(32),
                "tier-1 charge is 16 << 1 steps");
    menuFrame(LureZone::PICKER_BTN_B);
    flow.assert(LureZone::pickerOpen(), false, "the picker closes after a deposit");

    // --- gatherable encounter through a real step ---------------------------
    // The next committed step is the pocket's first encounter roll; the forced
    // table slot 2 is the authored spirit table's EARTH creature (species 0).
    gateCount = 0;
    forcedGateAt = 1;
    forcedSlot = 0;
    const bool stayedWorld = walk(Direction::RIGHT, 1);
    flow.assert(stayedWorld, false, "the lure-zone step starts an encounter");
    flow.assert(gameState.state, GameState_t::BATTLE, "the encounter enters battle");
    ++hud.encounters;
    const battle::BattleState &wild = battleSession().state();
    flow.assert(wild.gatherable, true, "a deposited lure makes the foe gatherable");
    flow.assert(wild.trainer, false, "the gatherable foe is an ordinary wild");
    flow.assert(wild.active[1].id, static_cast<uint8_t>(9),
                "the lure table selected the authored species");
    flow.assert(wild.active[1].level, static_cast<uint8_t>(3),
                "the inherited encounter level is the party average");
    flow.assert(wild.gather.need, NEED_LEVEL3, "the level-3 gather need is 9");
    flow.assert(wild.gather.tierRate, static_cast<uint8_t>(2),
                "the tier-1 lure gathers at rate 2");
    flow.assert(wild.gather.fleeTurns, static_cast<uint8_t>(6),
                "the authored flee window is six rounds");
    // The native host randomRoll stub returns out of range for a one-option
    // commit; the shipping RNG is bounded. Replay the same authored encounter
    // with the allowed deterministic session RNG before any round resolves.
    const uint8_t foeId = wild.active[1].id;
    const uint8_t foeLevel = wild.active[1].level;
    battleSession().setRng({flexibleBattleRng});
    battleSession().beginWild(foeId, foeLevel, true, 2);
    const uint8_t heroMax = battleSession().state().active[0].maxHp;
    flow.assert(heroMax, static_cast<uint8_t>(39), "fresh level-3 HP is 39");

    for (uint8_t round = 1; round <= 5; ++round) {
        gatherRound();
        if (round < 5) {
            flow.assert(battleSession().state().gather.progress,
                        static_cast<uint8_t>(round * 2),
                        "each gather round banks the authored rate");
            flow.assert(battleSession().state().active[0].hp,
                        static_cast<uint8_t>(heroMax - DUEL_HALF_WIN * round),
                        "the opponent's concealed turn costs the authored HP");
        }
    }
    flow.assert(gameState.state, GameState_t::WORLD,
                "the fifth gather acquires and returns to the world");
    flow.assert(player.creatureHPs[0],
                static_cast<uint8_t>(heroMax - DUEL_HALF_WIN * 5),
                "the acquired foe's turns cost 35 HP");
    ++hud.captures;
    StoreRecord gathered = {};
    flow.assert(storeRead(3, gathered), true, "the capture owns the next box slot");
    flow.assert(gathered.id, static_cast<uint8_t>(9), "the captured species is stored");
    flow.assert(levelFromExp(gathered.exp), static_cast<uint8_t>(3),
                "a captured instance keeps its level");
    flow.assert(storeInstanceHP(gathered), static_cast<uint8_t>(45),
                "a full-HP capture reads back at its derived maximum");
    flow.assert(occupiedSlots(), static_cast<uint8_t>(4),
                "one capture leaves four occupied instances (three party + one box)");

    // --- free rest ----------------------------------------------------------
    // The gatherable battle damaged slot 0; the authored rest point is a fresh
    // A on the faced (10,4) tile.
    flow.assert(player.creatureHPs[0] < player.party[0].hp, true,
                "the gather leaves the hero injured");
    face(Direction::DOWN, REST_X, REST_Y, TileProps::PROP_ENCOUNTER);
    pressA(REST_X, REST_Y, TileProps::PROP_ENCOUNTER);
    for (uint8_t i = 0; i < 3; ++i)
        flow.assert(player.creatureHPs[i], player.party[i].hp,
                    "the free rest restores every occupied member");

    // --- duplicate capture with a damaged boxed instance --------------------
    // Accelerated edge fixture: a real duel is still driven by button edges,
    // but the encounter entry is direct and the need is pinned to one round so
    // the boxed-HP encoding path runs inside the integration flow.
    player.healParty();
    enterBattle();
    gameState.state = GameState_t::BATTLE;
    battleSession().setRng({flexibleBattleRng});
    battleSession().beginWild(9, 3, true, 2);
    battle::BattleState &dup = battleSession().stateForTest();
    dup.gather.need = 1;
    dup.active[1].maxHp = 60;
    dup.active[1].hp = 55;
    gatherRound();
    flow.assert(gameState.state, GameState_t::WORLD, "the duplicate gather returns");
    StoreRecord duplicate = {};
    flow.assert(storeRead(4, duplicate), true, "a duplicate species owns its own slot");
    flow.assert(duplicate.id, static_cast<uint8_t>(9), "the duplicate keeps its species");
    flow.assert(duplicate.hpEncoded != 0, true,
                "a damaged boxed capture uses the non-zero HP encoding");
    flow.assert(storeInstanceHP(duplicate), static_cast<uint8_t>(55),
                "the boxed instance keeps its actual HP");
    ++hud.captures;
    flow.assert(item::inventoryCount(player.inventory,
                                     item::ItemKind::Consumable, 0),
                static_cast<uint8_t>(0),
                "a gatherable capture grants no authored heal");

    // Host fixture for the packed evolution columns (same contract as the save
    // suite): three parallel arrays anchored at the published source address.
    installEvolutionFixture();
    // --- ordinary wins to the exact level-8 evolution -----------------------
    // The authored first-area foe is the PLANT creature (species 12) with no
    // usable attack, so the ordinary win loop is deterministic; the foe level
    // tracks the clamped party average exactly as Encounter::select does.
    for (uint8_t win = 1; win <= 14; ++win) {
        // Accelerated checkpoint: the authored rest tile is proven above; the
        // 14-win ladder heals between battles so the exact XP/evolution edge
        // is measured without repeating the free-rest walk each time.
        player.healParty();
        uint8_t level = Encounter::partyAverageLevel(player.party);
        if (level < 2) level = 2;
        if (level > 5) level = 5;
        const uint16_t before = [&]() {
            StoreRecord record;
            storeRead(0, record);
            return record.exp;
        }();
        ordinaryWin(9, level);
        StoreRecord record = {};
        storeRead(0, record);
        flow.assert(record.exp, static_cast<uint16_t>(before + 8u * level),
                    "each ordinary win grants exactly 8 x opponent level");
        flow.assert(item::inventoryCount(player.inventory,
                                         item::ItemKind::Consumable, 0),
                    win, "each ordinary win banks the first authored heal once");
        flow.assert(item::inventoryCount(player.inventory,
                                         item::ItemKind::Consumable, 1),
                    win, "the second authored heal entry is granted too");
    }
    for (uint8_t i = 0; i < 3; ++i) {
        StoreRecord record = {};
        storeRead(i, record);
        flow.assert(record.exp, static_cast<uint16_t>(539),
                    "fourteen authored wins land on exact cumulative exp");
        flow.assert(levelFromExp(record.exp), static_cast<uint8_t>(8),
                    "the party reaches the authored level-8 evolution edge");
        flow.assert(player.party[i].level, static_cast<uint8_t>(8),
                    "the live party installs the derived level");
    }
    flow.assert(player.party[0].id, static_cast<uint8_t>(1),
                "species 0 evolves to 1 at level 8");
    flow.assert(player.party[1].id, static_cast<uint8_t>(4),
                "species 3 evolves to 4 at level 8");
    flow.assert(player.party[2].id, static_cast<uint8_t>(7),
                "species 6 evolves to 7 at level 8");
    flow.assert(EVOLVE_EXP, static_cast<uint16_t>(512),
                "the evolution edge is the authored 512 threshold");
    flow.assert(hud.wins, static_cast<uint8_t>(14),
                "the measured ordinary win count is inside the derived 13..31 band");

    // --- a Heal acquired by ordinary play, spent in a later duel ------------
    // The ladder's fourteen ordinary wins banked the authored drops; the last
    // win left the hero injured through the real damage path. This duel reads
    // and takes the Heal from the live bag (no injected inventory) and proves
    // the authored 16-point restore lands before the wild's committed hit.
    installConsumableFixture();
    const uint8_t healsBefore = item::inventoryCount(
        player.inventory, item::ItemKind::Consumable, 0);
    flow.assert(healsBefore, static_cast<uint8_t>(14),
                "fourteen ordinary wins bank fourteen authored heals");
    enterBattle();
    gameState.state = GameState_t::BATTLE;
    battleSession().setRng({flexibleBattleRng});
    battleSession().beginWild(9, 8, false, 0);
    attackRound(); // round 1: the wild lands its own hit on the hero
    const battle::BattleState &healDuel = battleSession().state();
    const uint8_t duelMaxHp = healDuel.active[0].maxHp;
    const uint8_t hpInjured = healDuel.active[0].hp;
    flow.assert(gameState.state, GameState_t::BATTLE,
                "the proof duel is still live after one round");
    flow.assert(hpInjured < duelMaxHp, true,
                "the ordinary duel leaves the hero injured");
    edge(0);               // choice entry builds the pick row
    edge(MENU_EDGE_B);     // non-gatherable wild: ITEMS is the first action row
    edge(MENU_EDGE_A);     // ITEMS opens the cached Heal bag
    flow.assert(item::inventoryCount(player.inventory,
                                     item::ItemKind::Consumable, 0),
                healsBefore, "opening the Heal page spends nothing");
    edge(MENU_EDGE_A);     // confirm the first authored Heal row
    flow.assert(item::inventoryCount(player.inventory,
                                     item::ItemKind::Consumable, 0),
                static_cast<uint8_t>(healsBefore - 1),
                "the duel takes exactly one authored heal");
    drainToChoiceOrWorld();
    const uint16_t healed =
        static_cast<uint16_t>(hpInjured) + static_cast<uint16_t>(HEAL_ARG0);
    const uint8_t clamped = healed > duelMaxHp ? duelMaxHp
                                               : static_cast<uint8_t>(healed);
    flow.assert(battleSession().state().damage[0] != 0, true,
                "the heal round still lands the wild's committed hit");
    flow.assert(battleSession().state().active[0].hp,
                static_cast<uint8_t>(clamped -
                                     battleSession().state().damage[0]),
                "the authored heal restores before that hit");
    escapeRound();
    flow.assert(gameState.state, GameState_t::WORLD,
                "ESCAPE returns the proof duel to the world");
    flow.assert(item::inventoryCount(player.inventory,
                                     item::ItemKind::Consumable, 0),
                static_cast<uint8_t>(healsBefore - 1),
                "an escaped duel grants no replacement heal");
    installZoneFixture(); // restore the packed encounter mirror for the walk

    // --- collection refusal and boxed replacement ---------------------------
    menuFrame(INVENTORY_MENU_B);
    flow.assert(collectionMenuWorldOpen(), true, "B opens the world root menu");
    menuFrame(INVENTORY_MENU_DOWN);
    menuFrame(INVENTORY_MENU_A);
    flow.assert(collectionMenuWorldOpen(), true, "A opens the collection page");
    menuFrame(INVENTORY_MENU_A);                 // select visible row 0 (party slot 0)
    menuFrame(INVENTORY_MENU_A);                 // assign to party slot 1
    flow.assert(collectionMenuWorldFeedback(), CollectionFeedback::InParty,
                "an already-in-party instance is refused without mutation");
    flow.assert(storePartySlot(0), static_cast<uint8_t>(0),
                "the refusal keeps the party reference");
    menuFrame(INVENTORY_MENU_DOWN);
    menuFrame(INVENTORY_MENU_DOWN);
    menuFrame(INVENTORY_MENU_DOWN);
    menuFrame(INVENTORY_MENU_A);                 // row 3 is the captured box slot
    flow.assert(collectionMenuWorldPage(), CollectionPage::Party,
                "A opens the party-slot page for a box instance");
    menuFrame(INVENTORY_MENU_DOWN);              // party slot 2
    menuFrame(INVENTORY_MENU_A);
    flow.assert(collectionMenuWorldFeedback(), CollectionFeedback::None,
                "a box instance replaces the chosen party slot");
    flow.assert(storePartySlot(1), static_cast<uint8_t>(3),
                "party slot 2 now references the captured instance");
    flow.assert(player.party[1].id, static_cast<uint8_t>(9),
                "the live replaced member loads the captured species");
    StoreRecord outgoing = {};
    flow.assert(storeRead(1, outgoing), true, "the outgoing instance stays boxed");
    flow.assert(outgoing.id, static_cast<uint8_t>(4),
                "the outgoing species remains in its own box slot");
    flow.assert(storeInstanceHP(outgoing), static_cast<uint8_t>(64),
                "the outgoing member's HP is mirrored, not healed");
    menuFrame(INVENTORY_MENU_B);
    menuFrame(INVENTORY_MENU_B);
    flow.assert(collectionMenuWorldOpen(), false, "B closes the world menu");

    // --- boss lose/retry and the fixed ending -------------------------------
    // Walk the authored route back to the boss approach at (20,9).
    const uint8_t healsBeforeBoss = item::inventoryCount(
        player.inventory, item::ItemKind::Consumable, 0);
    flow.assert(walk(Direction::DOWN, 2), true, "walk down to the route row");
    flow.assert(walk(Direction::RIGHT, 10), true, "walk the route east");
    flow.assert(walk(Direction::DOWN, 4), true, "walk the route south to the boss");
    face(Direction::RIGHT, BOSS_X, BOSS_Y, 0);
    pressA(BOSS_X, BOSS_Y, 0);
    flow.assert(gameState.state, GameState_t::BATTLE, "A on the boss starts the fight");
    ++hud.bossAttempts;
    flow.assert(battleSession().state().trainer, true, "the boss is a trainer");
    flow.assert(battleSession().state().active[1].id, static_cast<uint8_t>(31),
                "the fixed boss is species 31");
    flow.assert(battleSession().state().active[1].level, static_cast<uint8_t>(8),
                "the fixed boss is level 8");
    flow.assert(gameState.getFlag(flag_beta_complete), false,
                "entering the boss does not complete the area");

    {
        // Accelerated loss fixture: one live member, one hit, a PLANT hero the
        // boss's FIRE option beats. The real terminal/recovery path still runs.
        battle::BattleState &state = battleSession().stateForTest();
        state.partyCount[0] = 1;
        state.active[0].types =
            DualType(static_cast<Type>(static_cast<uint8_t>(Type::PLANT)));
        state.active[0].hp = state.active[0].maxHp = 1;
    }
    attackRound();
    drainFrames(4096);
    flow.assert(gameState.state, GameState_t::WORLD, "a boss loss returns to the world");
    flow.assert(gameState.getFlag(flag_beta_complete), false,
                "a boss loss never records completion");
    // The first world frame after the terminal applies the deferred recovery
    // before any input: full occupied party and the authored start.
    seedWindow(-1, -1, 0);
    worldInteractionFake::pickerButtons = 0;
    worldInteractionFake::pressedA = false;
    WorldEngine::runMap(worldState());
    ++hud.worldFrames;
    flow.assert(gameState.playerLocation, START_TILE,
                "defeat recovery returns to the authored start");
    flow.assert(player.creatureHPs[0], player.party[0].hp,
                "defeat recovery restores the occupied party to full HP");

    // Retry: walk back to the boss and win on a rigged one-HP boss.
    flow.assert(walk(Direction::RIGHT, 9), true, "retry walk east");
    flow.assert(walk(Direction::DOWN, 4), true, "retry walk south");
    face(Direction::RIGHT, BOSS_X, BOSS_Y, 0);
    pressA(BOSS_X, BOSS_Y, 0);
    flow.assert(gameState.state, GameState_t::BATTLE, "the boss is retryable");
    ++hud.bossAttempts;
    battleSession().stateForTest().active[1].hp = 1;
    attackRound();
    drainFrames(4096);
    flow.assert(gameState.state, GameState_t::WORLD, "the boss win returns to the world");
    flow.assert(gameState.getFlag(flag_beta_complete), true,
                "the boss win records the completion flag");
    flow.assert(item::inventoryCount(player.inventory,
                                     item::ItemKind::Consumable, 0),
                healsBeforeBoss,
                "a trainer loss and win grant no authored heal");
    face(Direction::RIGHT, BOSS_X, BOSS_Y, 0);
    pressA(BOSS_X, BOSS_Y, 0);
    flow.assert(gameState.state, GameState_t::WORLD,
                "a completed boss never restarts the fight");
    flow.assert(LureZone::messageAddr(), lureTextBetaComplete,
                "the completed boss shows the fixed ending line");

    // --- final damage battle for a non-default saved HP ---------------------
    const uint8_t beforeFinal = player.creatureHPs[0];
    ordinaryWin(9, 5);
    flow.assert(player.creatureHPs[0],
                static_cast<uint8_t>(beforeFinal - DUEL_HALF_CHIP * 3),
                "the final ordinary win costs the authored chip damage");

    // --- full bag: mature harvest is held, not lost -------------------------
    item::inventoryAdd(player.inventory, item::ItemKind::Lure, LURE_TIER1_TYPE0,
                       item::COUNT_MAX);
    for (uint8_t i = plants.plantStages.getStage(FIRST_BETA_PLOT_INDEX); i < 3; ++i)
        plants.plantStages.increment(FIRST_BETA_PLOT_INDEX);
    gameState.playerLocation = static_cast<uint16_t>((3 << 8) | 14);
    WorldEngine::init(worldState());
    WorldEngine::loadMap(worldState(), 0, 0);
    face(Direction::UP, PLOT_X, PLOT_Y, TileProps::PROP_PLANTABLE);
    pressA(PLOT_X, PLOT_Y, TileProps::PROP_PLANTABLE);
    flow.assert(item::inventoryCount(player.inventory, item::ItemKind::Lure,
                                     LURE_TIER1_TYPE0),
                item::COUNT_MAX, "a full bag saturates instead of wrapping");
    flow.assert(plants.plantStages.getStage(FIRST_BETA_PLOT_INDEX),
                static_cast<uint8_t>(3), "a refused full-bag harvest holds maturity");
    flow.assert(LureZone::messageAddr(), lureTextFull,
                "the full-bag harvest reports the bag is full");

    // --- explicit checkpoint: six live categories together ------------------
    uint16_t saveFrames = 0;
    flow.assert(runControllerSave(saveFrames), SaveStep::Done,
                "the explicit WORLD save completes");
    const uint16_t savedLocation = gameState.playerLocation;
    const uint8_t savedPlant = plants.plantStages.getStage(FIRST_BETA_PLOT_INDEX);
    const uint8_t savedHP = player.creatureHPs[0];
    const uint8_t savedBag = item::inventoryCount(
        player.inventory, item::ItemKind::Lure, LURE_TIER1_TYPE0);
    flow.assert(savedHP < player.party[0].hp, true,
                "the checkpoint carries a non-default party HP");

    player = Player();
    plants = PlantGameState{};
    gameState.playerLocation = 0;
    memset(gameState.flags, 0, sizeof(FLAG_BIT_ARRAY));
    flow.assert(SaveController::load(), true, "the explicit checkpoint reloads");
    flow.assert(gameState.playerLocation, savedLocation,
                "the reload restores the live location");
    flow.assert(plants.plantStages.getStage(FIRST_BETA_PLOT_INDEX), savedPlant,
                "the reload restores the plant stage");
    flow.assert(item::inventoryCount(player.inventory, item::ItemKind::Lure,
                                     LURE_TIER1_TYPE0),
                savedBag, "the reload restores the bag");
    flow.assert(player.creatureHPs[0], savedHP,
                "the reload restores the party HP");
    StoreRecord restored = {};
    flow.assert(storeRead(0, restored), true, "the reload restores the collection");
    flow.assert(restored.exp, static_cast<uint16_t>(579),
                "the reload restores exact stored experience");
    flow.assert(storeRead(3, restored), true, "the captured instance survives reload");
    flow.assert(restored.id, static_cast<uint8_t>(9),
                "the captured species survives reload");
    flow.assert(gameState.getFlag(flag_beta_complete), true,
                "the reload restores the completion flag");

    // --- interrupted save never half-publishes ------------------------------
    // Copy the committed checkpoint sectors so the cut can be compared without
    // a reboot (a reboot would reset the save step machine under the failure
    // screen; the save suites own that boot-wedge ordering).
    static uint8_t committedSectors[8192];
    memcpy(committedSectors, flashFakeData() + 0x2000, sizeof(committedSectors));
    item::inventoryClear(player.inventory);
    item::inventoryAdd(player.inventory, item::ItemKind::Lure, LURE_TIER1_TYPE0, 7);
    gameState.playerLocation = static_cast<uint16_t>((5 << 8) | 11);
    flashFakeSetWriteLimit(4096);
    SaveController::begin(GameState_t::WORLD);
    for (uint16_t frame = 0; frame < 1024 && gameState.state == GameState_t::SAVING;
         ++frame)
        SaveController::advance();
    flashFakeSetWriteLimit(-1);
    flow.assert(gameState.state, GameState_t::SAVING,
                "the cut save stays on the failure screen");
    const bool firstSectorIntact =
        memcmp(committedSectors, flashFakeData() + 0x2000, 4096) == 0;
    const bool secondSectorIntact =
        memcmp(committedSectors + 4096, flashFakeData() + 0x3000, 4096) == 0;
    flow.assert(firstSectorIntact && secondSectorIntact, true,
                "the interrupted save leaves the committed checkpoint sectors intact");

    // Fresh A retries the live snapshot and publishes the interrupted values.
    Arduboy2Base::currentButtonState = A_BUTTON;
    Arduboy2Base::previousButtonState = 0;
    SaveController::advance();
    Arduboy2Base::currentButtonState = 0;
    Arduboy2Base::previousButtonState = 0;
    for (uint16_t frame = 0; frame < 1024 && gameState.state == GameState_t::SAVING;
         ++frame)
        SaveController::advance();
    flow.assert(gameState.state, GameState_t::WORLD, "fresh A retries into the world");
    flow.assert(SaveController::load(), true, "the retried checkpoint reloads");
    flow.assert(item::inventoryCount(player.inventory, item::ItemKind::Lure,
                                     LURE_TIER1_TYPE0),
                static_cast<uint8_t>(7), "the retry publishes the live bag");
    flow.assert(gameState.playerLocation, static_cast<uint16_t>((5 << 8) | 11),
                "the retry publishes the live location");
    flow.assert(gameState.getFlag(flag_beta_complete), true,
                "the retry keeps the completed area");

    // Measured ordinary-control trace (recorded, not an accepted target).
    auto trace = [](const char *label, uint16_t value) {
        std::cout << "beta trace " << label << "=" << value << std::endl;
    };
    trace("steps", hud.steps);
    trace("worldFrames", hud.worldFrames);
    trace("battleFrames", hud.battleFrames);
    trace("encounters", hud.encounters);
    trace("wins", hud.wins);
    trace("captures", hud.captures);
    trace("bossAttempts", hud.bossAttempts);
    trace("evolutionLevel", player.party[0].level);
    trace("simulatedSeconds", static_cast<uint16_t>((hud.worldFrames + hud.battleFrames) / 52));
    suite.addTest(flow);

    // --- full 64-slot collection refuses a capture cleanly ------------------
    Test full64("A full 64-slot collection refuses a capture");
    freshGame();
    StoreRecord filler = {};
    filler.id = 12;
    filler.exp = levelExpThreshold(3);
    filler.attackMask = 0;
    for (uint8_t slot = 3; slot < 64; ++slot) {
        uint8_t assigned = 255;
        full64.assert(storeAdd(filler, assigned), true, "fill one collection slot");
        if (slot % 12 == 0) {
            SaveController::begin(GameState_t::WORLD);
            for (uint16_t frame = 0;
                 frame < 1024 && gameState.state == GameState_t::SAVING; ++frame)
                SaveController::advance();
        }
    }
    full64.assert(occupiedSlots(), static_cast<uint8_t>(64), "the collection is full");
    enterBattle();
    gameState.state = GameState_t::BATTLE;
    battleSession().beginWild(0, 3, true, 2);
    battleSession().stateForTest().gather.need = 1;
    gatherRound();
    full64.assert(gameState.state, GameState_t::WORLD,
                  "a full-store capture still ends cleanly");
    full64.assert(occupiedSlots(), static_cast<uint8_t>(64),
                  "a full store refuses without adding or wrapping a slot");
    StoreRecord first = {};
    StoreRecord last = {};
    full64.assert(storeRead(3, first), true, "the first filler is readable");
    full64.assert(storeRead(63, last), true, "the last filler is readable");
    full64.assert(first.id, static_cast<uint8_t>(12), "the first filler is untouched");
    full64.assert(last.id, static_cast<uint8_t>(12), "the last filler is untouched");

    // --- a near-full bag saturates the authored heal, never wraps -----------
    // The ordinary-win grant runs through inventoryAdd, so a 98-count heal plus
    // one KO clamps at COUNT_MAX and the second authored entry still lands.
    installEvolutionFixture(); // packed evolution mirror for the exp edge
    item::inventoryClear(player.inventory);
    item::inventoryAdd(player.inventory, item::ItemKind::Consumable, 0,
                       static_cast<uint8_t>(item::COUNT_MAX - 1));
    ordinaryWin(9, 3);
    full64.assert(item::inventoryCount(player.inventory,
                                       item::ItemKind::Consumable, 0),
                  item::COUNT_MAX, "a near-full authored heal saturates at 99");
    full64.assert(item::inventoryCount(player.inventory,
                                       item::ItemKind::Consumable, 1),
                  static_cast<uint8_t>(1),
                  "saturation of one entry still grants the other");
    suite.addTest(full64);

    runner.addTestSuite(suite);
}
