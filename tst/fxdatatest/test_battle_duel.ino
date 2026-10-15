#include "harness/fx_globals.hpp"
#include "battle_duel_test.hpp"
#include "duel_session_integration_test.hpp"

void setup() {
    fxTestSetup();
    FxTest test;
    battle_duel_device_test(test);   // pure kernel: 25 relations, damage, KO/draw, WAIT
    test_duel_session(test);         // real BattleSession via BattleFlow: 29/68 phases
    test.report(F("test_battle_duel"));
}
void loop() { exit(0); }
