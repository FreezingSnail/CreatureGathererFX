#define FX_GLOBALS_MINIMAL
#include "harness/fx_globals.hpp"
#include "encounter_test.hpp"

void setup() { fxTestSetup(); FxTest test; test_encounter(test); test.report(F("test_encounter")); }
void loop() { exit(0); }
