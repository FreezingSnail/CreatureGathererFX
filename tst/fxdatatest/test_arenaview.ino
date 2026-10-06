#include "harness/fx_globals.hpp"
#include "arenaview_test.hpp"

void setup() {
    fxTestSetup();
    FxTest test;
    test_arenaview(test);
    test.report(F("test_arenaview"));
}

void loop() { exit(0); }
