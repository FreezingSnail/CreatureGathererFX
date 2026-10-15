#include "harness/fx_globals.hpp"
#include "duel_resource_test.hpp"

void setup() {
    fxTestSetup();
    FxTest test;
    duel_resource_test(test); // painted stack + per-phase frame timing over the real chain
    test.report(F("test_duel_resources"));
}
void loop() { exit(0); }
