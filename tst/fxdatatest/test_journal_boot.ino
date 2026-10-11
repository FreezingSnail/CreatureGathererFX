#include "harness/fx_globals.hpp"
#include "journal_boot_test.hpp"

void setup() { fxTestSetup(); FxTest test; journal_boot_test::run(test); test.report(F("test_journal_boot")); }
void loop() { exit(0); }

