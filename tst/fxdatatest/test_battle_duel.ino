#include "harness/fx_globals.hpp"
#include "battle_duel_test.hpp"

// Pure RPS kernel proof: 25 relations, damage scaling, KO/draw, switch
// incoming-KO revert and WAIT (CreatureGathererFX-d73i.1). The real
// BattleSession/BattleFlow wiring lives in the co-located test_duel_session
// sketch (CreatureGathererFX-5j1n) so neither suite sits at the flash limit.
void setup() {
    fxTestSetup();
    FxTest test;
    battle_duel_device_test(test);
    test.report(F("test_battle_duel"));
}
void loop() { exit(0); }
