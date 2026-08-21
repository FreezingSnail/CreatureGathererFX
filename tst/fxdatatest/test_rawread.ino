#define FX_GLOBALS_MINIMAL
#include "harness/fx_globals.hpp"
#include "rawread_test.hpp"

void setup() { fxTestSetup(rawReadCursorFirstPage, rawReadCursorLastPage); FxTest test; test_rawread(test); test.report(F("test_rawread")); }
void loop() { exit(0); }
