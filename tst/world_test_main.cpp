#include "test.hpp"
#include "world_test.hpp"

#include "../src/GameState.hpp"
#include "../src/engine/ModeState.hpp"
#include "../src/plants/PlantGamestate.hpp"
#include "Arduboy2.h"

GameState gameState;
ModeState modeState;
PlantGameState plants;

bool worldInteractionJustPressedA() { return false; }
bool worldInteractionDialogActive() { return false; }
void worldInteractionPopDialog() {}
void worldInteractionReadScript(uint24_t, uint8_t *, uint8_t) {}
void worldInteractionRunScript(uint8_t *, uint16_t, uint16_t) {}
uint24_t worldInteractionScriptsBase() { return 0; }
uint8_t worldMovementButtons() { return 0; }

int main() {
    TestRunner tests;
    WorldSuite(tests);
    tests.printSummary();
    return tests.fail() ? 1 : 0;
}
