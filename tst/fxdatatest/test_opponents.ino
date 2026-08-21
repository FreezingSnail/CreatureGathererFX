#include "harness/fx_globals.hpp"
#include "opponents_test.hpp"

void setup() { fxTestSetup(); FxTest test; test_opponents(test); test.report(F("test_opponents")); }
void loop() { exit(0); }
