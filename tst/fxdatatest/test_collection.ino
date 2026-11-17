#include "harness/fx_globals.hpp"
#include "collection_test.hpp"

void setup() { fxTestSetup(); FxTest test; test_collection(test); test.report(F("test_collection")); }
void loop() { exit(0); }
