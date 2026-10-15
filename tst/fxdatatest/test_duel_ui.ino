#include "harness/fx_globals.hpp"
#include "duel_session_integration_test.hpp"
#include "duel_ui_test.hpp"

void setup() {
    fxTestSetup();
    FxTest test;
    // Renderer framebuffer geometry, outcome captions and the five-cell pick row.
    // The backend/session proof stays in test_battle_duel.ino.
    test_duel_ui(test);
    test.report(F("test_duel_ui"));
}
void loop() { exit(0); }
