#include "harness/fx_globals.hpp"
#include "win_exp_test.hpp"

void setup() { fxTestSetup(); FxTest test; test_win_exp(test); test.report(F("test_win_exp")); }
void loop() { exit(0); }
