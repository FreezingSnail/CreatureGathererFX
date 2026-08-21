#include "harness/fx_globals.hpp"
#include "moves_test.hpp"

void setup() { fxTestSetup(); FxTest test; test_moves(test); test.report(F("test_moves")); }
void loop() { exit(0); }
