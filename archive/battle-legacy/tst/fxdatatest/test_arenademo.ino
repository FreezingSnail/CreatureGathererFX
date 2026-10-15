#include "harness/fx_globals.hpp"
#include "arenademo_test.hpp"

void setup() {
    fxTestSetup();
    FxTest test;
    test_arenademo(test);
    test.report(F("test_arenademo"));
}

void loop() { exit(0); }
