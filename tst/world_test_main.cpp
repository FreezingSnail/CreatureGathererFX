#include "test.hpp"
#include "world_test.hpp"

#include "../src/GameState.hpp"
#include "../src/engine/ModeState.hpp"

GameState gameState;
ModeState modeState;

bool worldInteractionJustPressedA() { return false; }
bool worldInteractionDialogActive() { return false; }
void worldInteractionPopDialog() {}
void worldInteractionReadScript(uint24_t, uint8_t *, uint8_t) {}
void worldInteractionRunScript(uint8_t *, uint16_t, uint16_t) {}
uint24_t worldInteractionScriptsBase() { return 0; }

int main() {
    TestRunner tests;
    WorldSuite(tests);
    tests.printSummary();
    return tests.fail() ? 1 : 0;
}
