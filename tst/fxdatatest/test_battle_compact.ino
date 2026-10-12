#include "harness/fx_globals.hpp"
#include "battle_compact_test.hpp"

#ifndef CGFX_BATTLE_NO_FACTS
#error "compact result suite requires the no-facts resolver"
#endif
#ifndef CGFX_BATTLE_COMPACT_RESULT
#error "compact result suite must compile the compact result path"
#endif

void setup() {
    fxTestSetup();
    FxTest test;
    test_battle_compact(test);
    test.report(F("test_battle_compact"));
}

void loop() { exit(0); }
