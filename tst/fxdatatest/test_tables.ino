#define FX_GLOBALS_MINIMAL
#include "harness/fx_globals.hpp"
#include "tables_test.hpp"

void setup() { fxTestSetup(); FxTest test; test_tables(test); test.report(F("test_tables")); }
void loop() { exit(0); }
