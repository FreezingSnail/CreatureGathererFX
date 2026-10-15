#include "harness/fx_globals.hpp"
#include "trainer_setup_test.hpp"

void setup() {
    fxTestSetup();
    FxTest test;
    test_trainer_setup(test);
    test.report(F("test_trainer_setup"));
}

void loop() { exit(0); }
