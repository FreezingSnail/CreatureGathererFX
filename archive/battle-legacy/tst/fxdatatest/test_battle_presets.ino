#include "harness/fx_globals.hpp"
#include "preset_teams_test.hpp"

void setup() {
    fxTestSetup();
    FxTest test;
    test_preset_teams(test);
    test.report(F("test_battle_presets"));
}

void loop() { exit(0); }
