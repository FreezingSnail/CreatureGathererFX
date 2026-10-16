#include "harness/fx_globals.hpp"
#include "battlepresentation_test.hpp"

// Independent result-kind, effectiveness-caption, HP-track, move-label and
// attack-raster cases (CreatureGathererFX-5j1n split out of
// test_battlepresentation so both suites keep flash headroom). The end-to-end
// playback and painted-stack/frame-budget proofs stay in test_battlepresentation.
void setup() {
    fxTestSetup();
    FxTest test;
    test_battlepresentation_moves(test);
    test.report(F("test_battlepresentation_moves"));
}
void loop() { exit(0); }
