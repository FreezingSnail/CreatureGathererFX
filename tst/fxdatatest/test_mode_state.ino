#include "harness/fx_globals.hpp"
#include "mode_state_test.hpp"

void setup() { fxTestSetup(); FxTest test; test_mode_state(test); test.report(F("test_mode_state")); }
void loop() { exit(0); }
