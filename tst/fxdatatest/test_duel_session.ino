#include "harness/fx_globals.hpp"
#include "duel_session_integration_test.hpp"

// Real battle::BattleSession driven through BattleFlow::update: the Choice-entry
// concealed commitment, the 29-frame Concealed interval, the 68-frame Result
// dwell, the atomic round, WAIT and the gather ordering (CreatureGathererFX-5j1n
// split out of test_battle_duel). The pure kernel relations stay in
// test_battle_duel.
void setup() {
    fxTestSetup();
    FxTest test;
    test_duel_session(test);
    test.report(F("test_duel_session"));
}
void loop() { exit(0); }
