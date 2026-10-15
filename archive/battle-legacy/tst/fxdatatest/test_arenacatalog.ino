#include "harness/fx_globals.hpp"
#include "arenacatalog_test.hpp"

void setup() {
    fxTestSetup();
    FxTest test;
    test_arenacatalog(test);
    test.report(F("test_arenacatalog"));
}

void loop() { exit(0); }
