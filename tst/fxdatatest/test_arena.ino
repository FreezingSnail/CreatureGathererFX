#include "harness/fx_globals.hpp"
#include "arena_test.hpp"

void setup() { fxTestSetup(); FxTest test; test_arena(test); test.report(F("test_arena")); }
void loop() { exit(0); }
