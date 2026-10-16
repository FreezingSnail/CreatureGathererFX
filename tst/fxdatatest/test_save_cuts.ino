#include "harness/fx_globals.hpp"
#include "save_test.hpp"

// Interrupted-write "cut" proofs: journal/checkpoint publication and capture
// cuts (CreatureGathererFX-5j1n split out of test_save so both suites keep flash
// headroom). Each cut performs full checkpoint verify/erase cycles.
void setup() { fxTestSetup(); FxTest test; test_save_cuts(test); test.report(F("test_save_cuts")); }
void loop() { exit(0); }
