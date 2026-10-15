#include "harness/fx_globals.hpp"
#include "battlesession_test.hpp"

void setup() {
    fxTestSetup();
    FxTest test;
    test_battletrainer(test);
    test.report(F("test_battletrainer"));
}
void loop() { exit(0); }
