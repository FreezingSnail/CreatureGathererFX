#define FX_GLOBALS_MINIMAL
#include "harness/fx_globals.hpp"
#include "items_test.hpp"

void setup() { fxTestSetup(); FxTest test; test_items(test); test.report(F("test_items")); }
void loop() { exit(0); }
