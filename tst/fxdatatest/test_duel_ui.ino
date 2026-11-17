#include "harness/fx_globals.hpp"
#include "duel_session_integration_test.hpp"
#include "duel_ui_test.hpp"

// Capture-progress track (CreatureGathererFX-jp8.11.32): a 1px row at y47 below
// the GATHER cue, 32px wide, filled to round(32*progress/need). Absent for
// ordinary wilds, trainers and a fresh capture; the y48 pick row stays clear.
__attribute__((noinline)) void duelDeviceGatherTrack(FxTest &test) {
    using namespace duel_ui_test_detail;
    duelPrep(SpiritAuthored, SpiritAuthored, static_cast<uint8_t>(Type::WATER),
             SpiritAuthored, 52, 52);
    battle::BattleState &ordinary =
        const_cast<battle::BattleState &>(battleSession().state());
    ordinary.gather.need = 8;
    ordinary.gather.progress = 4; // render-only: an ordinary wild banks none
    BattleFlow::update(0);
    duelClearBuffer();
    renderBattle();
    test.expectEq(duelAnyPixel(68, 79, 47, 47), 0,
                  F("an ordinary wild shows no capture track"));

    beginWildDuel(true, 0); // gatherable wild, need 8 at level 1
    battle::BattleState &gatherable =
        const_cast<battle::BattleState &>(battleSession().state());
    gatherable.active[0].id = 255; // render-only: clear player sprite ink at y47
    gatherable.active[0].level = 0; // and the player badge frame at y47
    gatherable.gather.progress = 4; // 4/8 -> 16px
    BattleFlow::update(0);
    duelClearBuffer();
    renderBattle();
    test.expectEq(duelPixel(48, 47), 1, F("the track starts under the cue"));
    test.expectEq(duelPixel(63, 47), 1, F("half progress fills 16px"));
    test.expectEq(duelPixel(64, 47), 0, F("the fill stops at progress"));
    test.expectEq(duelPixel(79, 47), 0, F("half progress leaves the tail clear"));
    test.expectEq(duelPixel(5, 49), 1, F("the selected pick cell stays drawn"));

    gatherable.gather.progress = 8; // full capture
    duelClearBuffer();
    renderBattle();
    test.expectEq(duelPixel(79, 47), 1, F("a full capture fills all 32px"));
    test.expectEq(duelAnyPixel(80, 127, 47, 47), 0,
                  F("the track stays inside 32px"));

    gatherable.trainer = 1; // render-only: trainers never own the track
    duelClearBuffer();
    renderBattle();
    test.expectEq(duelAnyPixel(68, 79, 47, 47), 0,
                  F("a gatherable trainer shows no capture track"));

    gatherable.trainer = 0;
    gatherable.gather.progress = 0; // a fresh capture has no fill
    duelClearBuffer();
    renderBattle();
    test.expectEq(duelAnyPixel(48, 79, 47, 47), 0,
                  F("a fresh capture shows no capture track"));
}

void setup() {
    fxTestSetup();
    FxTest test;
    // Renderer framebuffer geometry, the six-cell pick row, the gatherable
    // centre cue, the GATHER action page and the outcome captions. The
    // backend/session proof stays in test_battle_duel.ino.
    test_duel_ui(test);
    duelDeviceGatherTrack(test);
    test.report(F("test_duel_ui"));
}
void loop() { exit(0); }
