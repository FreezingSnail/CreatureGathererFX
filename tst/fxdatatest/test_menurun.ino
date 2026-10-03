#include "harness/fx_globals.hpp"
#include "menurun_test.hpp"

void setup() {
    fxTestSetup();
    FxTest test;
    test_menurun(test);
    test.report(F("test_menurun"));
}

void loop() { exit(0); }
