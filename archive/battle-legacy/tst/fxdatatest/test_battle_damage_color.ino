#include "harness/fx_globals.hpp"
#include "battle_damage_color_test.hpp"

void setup() {
    fxTestSetup();
    FxTest test;
    test_battle_damage_color(test);
    test.report(F("test_battle_damage_color"));
}

void loop() { exit(0); }
