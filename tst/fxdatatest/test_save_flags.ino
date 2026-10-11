#include "harness/fx_globals.hpp"
#include "save_flags_test.hpp"

void setup() { fxTestSetup(); FxTest test; test_save_flags(test); test.report(F("test_save_flags")); }
void loop() { exit(0); }
