#include "harness/fx_globals.hpp"
#include "beta_area_test.hpp"

// WorldEngine::interact reaches the compact warp VM; the shipping sketch owns
// this global, so this suite must too once it drives a world frame.
ScriptVm vm;

// Ordinary first-area loop on the packed cart: real authored map, world,
// picker, gatherable duel, evolution, explicit SAVE and the fixed boss ending.
void setup() {
    fxTestSetup();
    FxTest test;
    test_beta_area(test);
}
void loop() { exit(0); }
