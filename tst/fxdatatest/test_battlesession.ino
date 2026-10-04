#include "harness/fx_globals.hpp"
#include "battlesession_test.hpp"

void setup() {
    fxTestSetup();
    FxTest test;
    test_battlesession(test);
    test.report(F("test_battlesession"));
}

void loop() { exit(0); }
