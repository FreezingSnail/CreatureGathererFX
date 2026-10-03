#include "moves_test.hpp"
#include "plantPair_test.hpp"
#include "plantStage_test.hpp"
#include "statModifier_test.hpp"
#include "creature_test.hpp"
#include "player_test.hpp"
#include "engine_test.hpp"
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
#include "mode_state_test.hpp"
#include "world_interact_test.hpp"
#include "step_test.hpp"
#include "fx_read_counter_test.hpp"
#include "battle_view_test.hpp"
#include "battle_test.hpp"
#include "battle_presentation_test.hpp"
#include "renderer_test.hpp"

#include "../src/globals.hpp"
#include <cstring>

Player player = Player();
GameState gameState;
ModeState modeState;
BattleEvent battleEventStack[10];
BattleEventPlayer battleEventPlayer;
MenuStack menuStack;
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
}
}

namespace worldMovementFake {
bool hasRequest = false;
Direction requestedDirection = Direction::DOWN;
uint16_t calls = 0;

void reset() {
    hasRequest = false;
    requestedDirection = Direction::DOWN;
    calls = 0;
}

void request(Direction direction) {
    requestedDirection = direction;
    hasRequest = true;
}

uint16_t inputCalls() { return calls; }
}

bool worldInteractionJustPressedA() { return worldInteractionFake::pressedA; }
bool worldInteractionDialogActive() { return dialogMenu.peek(); }
void worldInteractionPopDialog() { dialogMenu.popMenu(); }
uint24_t worldInteractionScriptsBase() { return worldInteractionFake::scriptsBase; }
bool worldMovementDirection(Direction &direction) {
    ++worldMovementFake::calls;
    if (!worldMovementFake::hasRequest) return false;
    worldMovementFake::hasRequest = false;
    direction = worldMovementFake::requestedDirection;
    return true;
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
    EngineSuite(tests);
    std::cout << "EngineSuite finished" << std::endl;
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
    ModeStateSuite(tests);
    std::cout << "ModeStateSuite finished" << std::endl;
    WorldInteractionSuite(tests);
    std::cout << "WorldInteractionSuite finished" << std::endl;
    StepSuite(tests);
    std::cout << "StepSuite finished" << std::endl;
    FxReadCounterSuite(tests);
    std::cout << "FxReadCounterSuite finished" << std::endl;
    BattleViewSuite(tests);
    std::cout << "BattleViewSuite finished" << std::endl;
    BattleSuite(tests);
    std::cout << "BattleSuite finished" << std::endl;
    BattlePresentationSuite(tests);
    std::cout << "BattlePresentationSuite finished" << std::endl;
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
