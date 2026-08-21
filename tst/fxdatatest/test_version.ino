#include "harness/fx_globals.hpp"
#include "version_test.hpp"

void setup() { fxTestSetup(); FxTest test; test_version(test); test.report(F("test_version")); }
void loop() { exit(0); }
