#include "harness/fx_globals.hpp"
#include "stack_test.hpp"

ScriptVm vm;

void setup() {
    fxTestSetup();
    FxTest test;
    stack_fx_test_detail::measureModeTransition(test);
    test_stack(test);
    test.report(F("test_stack"));
}
void loop() { exit(0); }
