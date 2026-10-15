#include "moves_test.hpp"
#include "plantPair_test.hpp"
#include "plantStage_test.hpp"
#include "statModifier_test.hpp"
#include "creature_test.hpp"
#include "player_test.hpp"
#include "opponent_test.hpp"
#include "type_test.hpp"
#include "effect_test.hpp"
#include "battle_effects_test.hpp"
#include "battle_setup_test.hpp"
#include "gamestate_test.hpp"
#include "save_test.hpp"
#include "item_test.hpp"
#include "listview_test.hpp"
#include "menu_test.hpp"
#include "chunk_test.hpp"
#include "dialog_test.hpp"
// mode_state/transition drive the full battle flow and assume legacy immediate
// resolution; the duel build adds 29/68-frame timing, so they belong to the
// legacy contract run (no macro).
#ifndef CGFX_DUEL_BATTLE
#include "mode_state_test.hpp"
#endif
#include "world_interact_test.hpp"
#include "step_test.hpp"
#include "encounter_test.hpp"
#include "lure_zone_test.hpp"
#ifndef CGFX_DUEL_BATTLE
#include "transition_test.hpp"
#endif
#include "fx_read_counter_test.hpp"
#include "battle_view_test.hpp"
#include "battle_test.hpp"
#include "battle_ai_test.hpp"
#include "battle_resolve_test.hpp"
#include "battle_faint_test.hpp"
#include "battle_presentation_test.hpp"
#include "battle_hp_spike_test.hpp"
#include "renderer_test.hpp"
#include "blit_test.hpp"

#include "../src/globals.hpp"
#include "../src/engine/menu/MenuV2.hpp"
#include <cstring>
// battle_session/flow/slim_session assert the legacy sequential two-actor
// contract; the CGFX_DUEL_BATTLE build replaces that path, so they belong to
// the legacy contract run (no macro).
#ifndef CGFX_DUEL_BATTLE
#include "battle_session_test.hpp"
#include "battle_playback_test.hpp"
#include "battle_flow_test.hpp"
#endif
#include "arena_lifecycle_test.hpp"
#include "arena_catalog_test.hpp"
#include "arena_navigation_test.hpp"
#include "arena_view_test.hpp"
#include "arena_flow_test.hpp"
#include "battle_presets_test.hpp"
#include "battle_item_test.hpp"
// battle_utility asserts the PP/use-bank contract; the duel has no PP and
// aliases moveUsesSpent, so it belongs to the legacy contract run (no macro).
#ifndef CGFX_DUEL_BATTLE
#include "battle_utility_test.hpp"
#endif
#include "battle_slim_test.hpp"
#ifndef CGFX_DUEL_BATTLE
#include "battle_slim_session_test.hpp"
// battle_slim_menu asserts the legacy options/move/replacement pages, which the
// CGFX_DUEL_BATTLE build replaces with the compact duel pick/picker/action pages.
#include "battle_slim_menu_test.hpp"
// battle_slim_feedback pins the legacy scene raster tail (player frame last) and
// the legacy feedback footer; the duel scene appends badges/pips/footer, so it is
// part of the legacy contract run.
#include "battle_slim_feedback_test.hpp"
#endif
#ifdef BATTLE_SIMULATOR
#include "battle_sim_state_test.hpp"
#include "battle_sim_runner_test.hpp"
#endif
#ifdef CGFX_DUEL_BATTLE
#include "duel_kernel_test.hpp"
#include "duel_session_integration_test.hpp"
#include "duel_pick_row_test.hpp"
#include "duel_view_contract_test.hpp"
#include "duel_menu_test.hpp"
#endif

Player player = Player();
GameState gameState;
ModeState modeState;
MenuV2 menu;
DialogMenu dialogMenu;
PlantGameState plants;
uint8_t screenBuffer[128 * 64];
uint8_t *buffer = screenBuffer;

namespace worldInteractionFake {
bool pressedA = false;
uint24_t scriptsBase = 0x12000;
uint8_t slotBytes[128] = {};
uint16_t readCount = 0;
uint24_t lastAddress = 0;
uint8_t lastLength = 0;
uint8_t *lastScript = nullptr;
uint16_t runCount = 0;
uint16_t currentTile = 0;
uint16_t targetTile = 0;
uint8_t runFirstByte = 0;
uint8_t pickerButtons = 0;

void reset() {
    pressedA = false;
    std::memset(slotBytes, 0, sizeof(slotBytes));
    readCount = 0;
    lastAddress = 0;
    lastLength = 0;
    lastScript = nullptr;
    runCount = 0;
    currentTile = 0;
    targetTile = 0;
    runFirstByte = 0;
    pickerButtons = 0;
}
}

namespace worldMovementFake {
uint8_t pendingButtons = 0;
uint16_t calls = 0;

void reset() {
    pendingButtons = 0;
    calls = 0;
}

void request(Direction direction) {
    switch (direction) {
    case Direction::LEFT: pendingButtons = LEFT_BUTTON; break;
    case Direction::RIGHT: pendingButtons = RIGHT_BUTTON; break;
    case Direction::UP: pendingButtons = UP_BUTTON; break;
    case Direction::DOWN: pendingButtons = DOWN_BUTTON; break;
    }
}

void requestButtons(uint8_t buttons) { pendingButtons = buttons; }

uint16_t inputCalls() { return calls; }
}

bool worldInteractionJustPressedA() { return worldInteractionFake::pressedA; }
bool worldInteractionDialogActive() { return dialogMenu.peek(); }
void worldInteractionPopDialog() { dialogMenu.popMenu(); }
uint24_t worldInteractionScriptsBase() { return worldInteractionFake::scriptsBase; }
uint8_t worldPickerButtons() { return worldInteractionFake::pickerButtons; }
uint8_t worldMovementButtons() {
    ++worldMovementFake::calls;
    const uint8_t buttons = worldMovementFake::pendingButtons;
    worldMovementFake::pendingButtons = 0;
    return buttons;
}
void worldInteractionReadScript(uint24_t address, uint8_t *dst, uint8_t length) {
    ++worldInteractionFake::readCount;
    worldInteractionFake::lastAddress = address;
    worldInteractionFake::lastLength = length;
    worldInteractionFake::lastScript = dst;
    std::memcpy(dst, worldInteractionFake::slotBytes, length);
}
void worldInteractionRunScript(uint8_t *script, uint16_t currentTile, uint16_t targetTile) {
    ++worldInteractionFake::runCount;
    worldInteractionFake::currentTile = currentTile;
    worldInteractionFake::targetTile = targetTile;
    worldInteractionFake::runFirstByte = script[0];
}

int main() {
    std::cout << "Starting Runner" << std::endl;
    TestRunner tests;
    std::cout << "Starting Tests" << std::endl;

    // Run test suites
    MoveSuite(tests);
    std::cout << "MoveSuite finished" << std::endl;
    TypeSuite(tests);
    std::cout << "TypeSuite finished" << std::endl;
    PlantPairSuite(tests);
    std::cout << "PlantPairSuite finished" << std::endl;
    PlantStageSuite(tests);
    std::cout << "PlantStageSuite finished" << std::endl;
    ModifierSuite(tests);
    std::cout << "ModifierSuite finished" << std::endl;
    CreatureSuite(tests);
    std::cout << "CreatureSuite finished" << std::endl;
    PlayerSuite(tests);
    std::cout << "PlayerSuite finished" << std::endl;
    OpponentSuite(tests);
    std::cout << "OpponentSuite finished" << std::endl;
    EffectSuite(tests);
    std::cout << "EffectSuite finished" << std::endl;
    BattleEffectsSuite(tests);
    std::cout << "BattleEffectsSuite finished" << std::endl;
    BattleSetupSuite(tests);
    std::cout << "BattleSetupSuite finished" << std::endl;
    GameStateSuite(tests);
    std::cout << "GameStateSuite finished" << std::endl;
    SaveSuite(tests);
    std::cout << "SaveSuite finished" << std::endl;
    ItemSuite(tests);
    std::cout << "ItemSuite finished" << std::endl;
    ListViewSuite(tests);
    std::cout << "ListViewSuite finished" << std::endl;
    MenuNavSuite(tests);
    std::cout << "MenuNavSuite finished" << std::endl;
    ChunkSuite(tests);
    std::cout << "ChunkSuite finished" << std::endl;
    DialogSuite(tests);
    std::cout << "DialogSuite finished" << std::endl;
#ifndef CGFX_DUEL_BATTLE
    ModeStateSuite(tests);
    std::cout << "ModeStateSuite finished" << std::endl;
#endif
    WorldInteractionSuite(tests);
    std::cout << "WorldInteractionSuite finished" << std::endl;
    StepSuite(tests);
    std::cout << "StepSuite finished" << std::endl;
    EncounterSuite(tests);
    std::cout << "EncounterSuite finished" << std::endl;
    LureZoneSuite(tests);
    std::cout << "LureZoneSuite finished" << std::endl;
#ifndef CGFX_DUEL_BATTLE
    TransitionSuite(tests);
    std::cout << "TransitionSuite finished" << std::endl;
#endif
    FxReadCounterSuite(tests);
    std::cout << "FxReadCounterSuite finished" << std::endl;
    BattleViewSuite(tests);
    std::cout << "BattleViewSuite finished" << std::endl;
    BattleSuite(tests);
    std::cout << "BattleSuite finished" << std::endl;
    BattleAiSuite(tests);
    std::cout << "BattleAiSuite finished" << std::endl;
    BattleResolveSuite(tests);
    std::cout << "BattleResolveSuite finished" << std::endl;
    BattleFaintSwitchSuite(tests);
    std::cout << "BattleFaintSwitchSuite finished" << std::endl;
    BattlePresentationSuite(tests);
    BattleHpSpikeSuite(tests);
    std::cout << "BattlePresentationSuite finished" << std::endl;
#ifndef CGFX_DUEL_BATTLE
    BattleSessionSuite(tests);
    std::cout << "BattleSessionSuite finished" << std::endl;
    // The compact flow suite covers single-phase feedback and action boundaries.
    BattleFlowSuite(tests);
    std::cout << "Slim battle frame controller finished" << std::endl;
#endif
    ArenaLifecycleSuite(tests);
    ArenaCatalogSuite(tests);
    ArenaNavigationSuite(tests);
    ArenaViewSuite(tests);
    ArenaFlowSuite(tests);
    BattlePresetsSuite(tests);
    BattleItemSuite(tests);
#ifndef CGFX_DUEL_BATTLE
    BattleUtilitySuite(tests);
#endif
    battle_slim_fixture::BattleSlimScaffoldSuite(tests);
#ifndef CGFX_DUEL_BATTLE
    battle_slim_session_test_detail::BattleSlimSessionSuite(tests);
    // The legacy options/move/replacement menu contract is replaced by the duel
    // pages, so it is not part of the duel build.
    BattleSlimMenuSuite(tests);
    battle_slim_feedback_fixture::SlimFeedbackSuite(tests);
#endif
#ifdef CGFX_DUEL_BATTLE
    std::cout << "DuelKernelSuite" << std::endl;
    DuelKernelSuite(tests);
    std::cout << "DuelSessionIntegrationSuite" << std::endl;
    DuelSessionIntegrationSuite(tests);
    std::cout << "DuelPickRowSuite" << std::endl;
    DuelPickRowSuite(tests);
    std::cout << "DuelViewContractSuite" << std::endl;
    {
        TestSuite duelViewSuite("Duel view contract");
        DuelViewContractSuite(duelViewSuite);
        tests.addTestSuite(duelViewSuite);
    }
    std::cout << "DuelMenuSuite" << std::endl;
    {
        TestSuite duelMenuSuite("Duel menu pages");
        DuelMenuSuite(duelMenuSuite);
        tests.addTestSuite(duelMenuSuite);
    }
#endif
#ifdef BATTLE_SIMULATOR
    BattleSimulatorStateSuite(tests);
    BattleSimulatorRunnerSuite(tests);
#endif
    std::cout << "BattleItemSuite finished" << std::endl;
    BlitSuite(tests);
    NativeRendererSuite(tests);
    std::cout << "NativeRendererSuite finished" << std::endl;
    std::cout << "Tests Finished" << std::endl;
    tests.printSummary();
    if (tests.fail()) {
        std::cout << "Tests failed!" << std::endl;
        return 1;
    } else {
        std::cout << "All tests passed!" << std::endl;
    }
    return 0;
}
