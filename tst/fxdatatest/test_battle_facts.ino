#include "harness/fx_globals.hpp"
#include "battle_facts_test.hpp"

#ifndef CGFX_BATTLE_NO_FACTS
#error "test_battle_facts must compile the no-facts resolver path"
#endif

void setup() { fxTestSetup(); FxTest test; test_battle_facts(test); test.report(F("test_battle_facts")); }
void loop() { exit(0); }
