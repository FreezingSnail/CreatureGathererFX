#define FX_GLOBALS_MINIMAL
#include "harness/fx_globals.hpp"
#include "src/GameState.hpp"
#include "src/engine/world/World.hpp"
#include "tiles_test.hpp"

GameState gameState;
ModeState modeState;

void setup() {
    fxTestSetup();
    FxTest test;
    test_tiles(test);
    test.report(F("test_tiles"));
}

void loop() { exit(0); }
