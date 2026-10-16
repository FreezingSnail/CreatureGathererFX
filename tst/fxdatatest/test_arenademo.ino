#include "harness/fx_globals.hpp"
#include "arenademo_test.hpp"

// Random-match arena demo (CreatureGathererFX-lyd8): boot rolls a random player
// team + opponent trainer from the shared RNG and starts immediately, and every
// finishBattle re-rolls the pair. rngSeed() pins the sequence so the draws are
// exact; a device run proves the real FX catalog accepts the rolled pair.
void setup() {
    fxTestSetup();
    FxTest test;
    test_arenademo(test);
    test.report(F("test_arenademo"));
}

void loop() { exit(0); }
