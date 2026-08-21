#include "harness/fx_globals.hpp"
#include "creatures_test.hpp"

void setup() { fxTestSetup(); FxTest test; test_creatures(test); test.report(F("test_creatures")); }
void loop() { exit(0); }
