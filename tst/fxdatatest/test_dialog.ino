#include "harness/fx_globals.hpp"
#include "dialogs_test.hpp"

void setup() {
    fxTestSetup();
    FxTest test;
    test_dialogs(test);
    test.report(F("test_dialog"));
}

void loop() { exit(0); }
