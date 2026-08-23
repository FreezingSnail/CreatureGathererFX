#include "harness/fx_globals.hpp"
#include "save_test.hpp"

void setup() { fxTestSetup(); FxTest test; test_save(test); test.report(F("test_save")); }
void loop() { exit(0); }
