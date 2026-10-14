#define FX_GLOBALS_MINIMAL
#include "harness/fx_globals.hpp"
#include "plant_stage_test.hpp"

void setup() {
    fxTestSetup();
    FxTest test;
    test_plant_stage(test);
    test.report(F("test_plant_stage"));
}

void loop() { exit(0); }
