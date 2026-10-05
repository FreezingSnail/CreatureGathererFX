#define FX_GLOBALS_MINIMAL
#include "harness/fx_globals.hpp"
#include "src/player/Player.hpp"
Player player;
#include "battle_utility_test.hpp"

void setup() {
    fxTestSetup();
    FxTest test;
    test_battleutility(test);
    test.report(F("test_battleutility"));
}

void loop() { exit(0); }
