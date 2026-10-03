#include "harness/fx_globals.hpp"
#include "readcounter_test.hpp"

void setup() { fxTestSetup(); FxTest test; test_readcounter(test); test.report(F("test_readcounter")); }
void loop() { exit(0); }
