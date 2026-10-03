#include "harness/fx_globals.hpp"
#include "scripts_test.hpp"

ScriptVm vm;

void setup() {
    fxTestSetup();
    FxTest test;
    test_scripts(test, vm, modeState.world.script);
    test.report(F("test_scripts"));
}

void loop() { exit(0); }
