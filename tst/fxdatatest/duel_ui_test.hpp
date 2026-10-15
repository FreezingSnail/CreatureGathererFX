#pragma once
// Device framebuffer + outcome-caption proof for the compact duel HUD
// (CreatureGathererFX-d73i.2). Split out of the backend/session suite so
// test_battle_duel keeps program-flash headroom; wired by
// tst/fxdatatest/test_duel_ui.ino, which the Makefile builds with the shipping
// compact duel profile (CGFX_DUEL_BATTLE + COMPACT_RESULT + NO_FACTS +
// FX_READ_COUNTER). Shares the fixture helpers in duel_session_integration_test.
#ifdef CGFX_DUEL_BATTLE

#include "fxtest.hpp"
#include "src/engine/battle/DuelView.hpp"
#include "duel_session_integration_test.hpp"

namespace duel_ui_test_detail {

using namespace duel_session_test_detail;

// Shared framing prologue for the framebuffer/HUD tests: a non-gatherable wild
// duel with a fully known matchup, so the fixtures stay small.
__attribute__((noinline)) void duelPrep(uint8_t playerType, uint8_t playerMove,
                                        uint8_t foeType, uint8_t foeMove,
                                        uint8_t playerHp, uint8_t foeHp) {
    preparePlayer();
    beginWildDuel(false, 0);
    configureMatchup(playerType, playerMove, foeType, foeMove, playerHp, foeHp);
}

void duelClearBuffer() {
    for (uint16_t i = 0; i < sizeof(Arduboy2Base::sBuffer); ++i)
        Arduboy2Base::sBuffer[i] = 0;
}

bool duelPixel(uint8_t x, uint8_t y) {
    return ((arduboy.getBuffer()[x + static_cast<uint16_t>(y >> 3) * 128u] >>
             (y & 7)) & 1) != 0;
}

bool duelAnyPixel(uint8_t x0, uint8_t x1, uint8_t y0, uint8_t y1) {
    for (uint8_t y = y0; y <= y1; ++y)
        for (uint8_t x = x0; x <= x1; ++x)
            if (duelPixel(x, y)) return true;
    return false;
}

// A surviving full gather acquires: the terminal Outcome::Gathered owns the
// footer caption ("gathered"), not the in-progress round spelling.
void duelDeviceGatherAcquisition(FxTest &test) {
    preparePlayer();
    beginWildDuel(true, 8); // gatherable, need 8, tierRate 8
    configureMatchup(SpiritAuthored, SpiritAuthored, FireAuthored, SpiritAuthored,
                     100, 100);

    test.expectEq(battleSession().submitIntent({MenuIntentKind::Gather, 0}), 1,
                  F("a wild gather submits"));
    uint16_t revealTicks = 0;
    test.expectEq(runToResult(battleSession(), revealTicks), 1,
                  F("the surviving gather reveals"));
    test.expectEq(battleSession().state().gather.progress, 8,
                  F("progress reaches the need"));
    test.expectEq(static_cast<uint32_t>(battleSession().result().outcome),
                  static_cast<uint32_t>(battle::Outcome::Gathered),
                  F("a surviving full gather is acquired"));
    test.expectEq(battleSession().state().feedback,
                  static_cast<uint8_t>(duel::Feedback::Gathered),
                  F("the footer shows the gathered caption"));
}

// A surviving wild round whose flee countdown hits zero is Fled, and the footer
// shows the fled caption (not a generic gather/round spelling).
void duelDeviceFleeCaption(FxTest &test) {
    preparePlayer();
    beginWildDuel(true, 0); // gatherable, tierRate 0 -> progress never reaches need
    battle::BattleState &state =
        const_cast<battle::BattleState &>(battleSession().state());
    state.gather.need = 8;
    state.gather.fleeTurns = 1;
    configureMatchup(FireAuthored, FireAuthored, FireAuthored, FireAuthored, 100, 100);

    test.expectEq(battleSession().submitIntent({MenuIntentKind::SelectMove, 0}), 1,
                  F("a wild attack submits"));
    uint16_t revealTicks = 0;
    test.expectEq(runToResult(battleSession(), revealTicks), 1, F("the round reveals"));
    test.expectEq(static_cast<uint32_t>(battleSession().result().outcome),
                  static_cast<uint32_t>(battle::Outcome::Fled),
                  F("a spent flee countdown flees"));
    test.expectEq(battleSession().state().feedback,
                  static_cast<uint8_t>(duel::Feedback::Fled),
                  F("the footer shows the fled caption"));
    test.expectEq(battleSession().state().gather.progress, 0,
                  F("an attack round banks no gather progress"));
}

// Badge ownership + disjoint HP numerals/bar. Player active spirit (icon 0), foe
// water (icon 1): the two icons differ at icon-local (0,2) and (1,2), so a
// swapped badge side fails.
void duelDeviceBadgeAndHp(FxTest &test) {
    duelPrep(SpiritAuthored, SpiritAuthored, static_cast<uint8_t>(Type::WATER),
             SpiritAuthored, 52, 52);
    BattleFlow::update(0); // choice entry -> pick row
    duelClearBuffer();
    renderBattle();

    // Player chips read bottom-right (x = 113 - i*12), foe bottom-left (1 + i*12).
    test.expectEq(duelPixel(114, 40), 1, F("player badge shows its own type icon"));
    test.expectEq(duelPixel(115, 40), 0, F("player badge glyph is spirit, not water"));
    test.expectEq(duelPixel(2, 40), 0, F("foe badge glyph is water, not spirit"));
    test.expectEq(duelPixel(3, 40), 1, F("foe badge shows its own type icon"));

    // Foe numerals hug the panel top (rows 1..5); the y=10 bar is disjoint.
    test.expectEq(duelAnyPixel(50, 57, 1, 5), 1, F("foe numerals are drawn"));
    test.expectEq(duelAnyPixel(50, 57, 6, 9), 0, F("foe numerals stay clear of the bar"));
    test.expectEq(duelPixel(50, 11), 1, F("foe HP bar is drawn"));
    test.expectEq(duelAnyPixel(50, 57, 27, 31), 1, F("player numerals are drawn"));
    test.expectEq(duelPixel(50, 36), 1, F("player HP bar is drawn"));
}

// Three-digit HP numerals (up to x77) stay legible while all three pips are
// drawn on the free rows (foe y13, player y21).
void duelDeviceThreeDigitHp(FxTest &test) {
    duelPrep(SpiritAuthored, SpiritAuthored, static_cast<uint8_t>(Type::WATER),
             SpiritAuthored, 254, 254);
    battle::BattleState &state =
        const_cast<battle::BattleState &>(battleSession().state());
    state.partyCount[0] = 3;
    state.partyCount[1] = 3; // render-only: give the foe two living bench cells
    state.bench[0][0].hp = 10;
    state.bench[0][1].hp = 10;
    state.bench[1][0].hp = 10;
    state.bench[1][1].hp = 10;
    BattleFlow::update(0);
    duelClearBuffer();
    renderBattle();

    test.expectEq(duelAnyPixel(74, 77, 1, 5), 1, F("foe three-digit numerals drawn"));
    test.expectEq(duelAnyPixel(72, 79, 13, 17), 1, F("foe pips drawn below the numerals"));
    test.expectEq(duelAnyPixel(72, 79, 6, 9), 0, F("foe numeral/pip rows stay disjoint"));
    test.expectEq(duelPixel(72, 21), 1, F("player first pip drawn"));
    test.expectEq(duelPixel(78, 21), 1, F("player third pip drawn"));
    test.expectEq(duelAnyPixel(74, 77, 27, 31), 1, F("player three-digit numerals drawn"));
}

// Five-cell pick row + centre strip geometry: four attacks + SWAP, no overflow.
void duelDevicePickRowGeometry(FxTest &test) {
    duelPrep(SpiritAuthored, SpiritAuthored, static_cast<uint8_t>(Type::WATER),
             SpiritAuthored, 52, 52);
    battle::BattleState &state =
        const_cast<battle::BattleState &>(battleSession().state());
    state.partyCount[0] = 3;
    state.bench[0][0].hp = 10;
    state.bench[0][1].hp = 10;
    for (uint8_t slot = 0; slot < 4; ++slot) {
        state.active[0].moveTypes[slot] = SpiritAuthored;
        state.active[0].moveIds[slot] = slot;
    }
    BattleFlow::update(0); // choice entry -> pick row
    duelClearBuffer();
    renderBattle();

    // Four attacks + SWAP = five cells: cell = 24, boxes at x = 4, 28, 52, 76, 100.
    test.expectEq(duelPixel(5, 49), 1, F("selected first cell is a white box"));
    test.expectEq(duelPixel(26, 49), 0, F("gap between cells stays black"));
    test.expectEq(duelAnyPixel(100, 121, 48, 61), 1, F("SWAP cell draws its label"));
    test.expectEq(duelAnyPixel(122, 127, 48, 61), 0, F("row stays inside 122px"));
    // Centre strip stays within x48..79; the selected icon ends by x65.
    test.expectEq(duelAnyPixel(48, 56, 41, 45), 1, F("centre R# is drawn"));
    test.expectEq(duelAnyPixel(57, 64, 40, 47), 1, F("centre selected type icon drawn"));
    test.expectEq(duelAnyPixel(66, 79, 41, 47), 0, F("centre stays left of the sprite"));
}

// Result footer: both revealed move icons, the caption, and the player-outgoing
// damage figure printed before the foe-outgoing one.
void duelDeviceFeedback(FxTest &test) {
    preparePlayer();
    beginWildDuel(false, 0);
    // Player creature fire / move fire; foe creature water / move spirit. Fire
    // loses to spirit, so the foe wins: 14 x1.5 (spirit beats fire) = 21 to the
    // player, while the player's chip 4 x0.5 (fire loses to water) = 2.
    configureMatchup(FireAuthored, FireAuthored, static_cast<uint8_t>(Type::WATER),
                     SpiritAuthored, 100, 100);
    test.expectEq(battleSession().submitIntent({MenuIntentKind::SelectMove, 0}), 1,
                  F("the losing attack submits"));
    uint16_t revealTicks = 0;
    test.expectEq(runToResult(battleSession(), revealTicks), 1, F("the round reveals"));
    test.expectEq(battleSession().state().damage[1], 2, F("player dealt 2 (foe slot)"));
    test.expectEq(battleSession().state().damage[0], 21, F("foe dealt 21 (player slot)"));
    duelClearBuffer();
    renderBattle();

    test.expectEq(duelAnyPixel(6, 13, 50, 57), 1, F("player revealed move icon drawn"));
    test.expectEq(duelAnyPixel(30, 37, 50, 57), 1, F("foe revealed move icon drawn"));
    test.expectEq(duelAnyPixel(42, 96, 48, 55), 1, F("feedback caption drawn"));
    // First figure (player outgoing 2) is one digit: nothing past x=48.
    test.expectEq(duelAnyPixel(50, 58, 56, 60), 0, F("first figure is the one-digit player hit"));
    // Second figure (foe outgoing 21) has a second digit near x=73.
    test.expectEq(duelAnyPixel(72, 80, 56, 60), 1, F("second figure is the two-digit foe hit"));
}

// Dual-type badges: spirit+water draws two chips; wind+water folds to water and
// deduplicates to a single chip. Player chips read bottom-right at x=113 (i=0)
// and x=101 (i=1), so pixel(100,36) distinguishes two chips from one.
void duelDeviceDualBadges(FxTest &test) {
    duelPrep(SpiritAuthored, SpiritAuthored, static_cast<uint8_t>(Type::WATER),
             SpiritAuthored, 52, 52);
    battle::BattleState &state =
        const_cast<battle::BattleState &>(battleSession().state());
    // Clear the player sprite art so the badge region is a black background a
    // chip frame alone can light up (the sprite mask would otherwise set pixels).
    state.active[0].id = 255;
    state.active[0].types = DualType(Type::SPIRIT, Type::WATER);
    BattleFlow::update(0);
    duelClearBuffer();
    renderBattle();
    test.expectEq(duelPixel(100, 36), 1, F("dual type draws a second player chip"));

    state.active[0].types = DualType(Type::WIND, Type::WATER);
    duelClearBuffer();
    renderBattle();
    test.expectEq(duelPixel(100, 36), 0, F("wind+water deduplicates to one chip"));
}

// Empty/KO pips: a living active draws its pip and the active underline, a KO'd
// bench cell draws nothing, and a living bench cell draws its own pip.
void duelDeviceKoPips(FxTest &test) {
    duelPrep(SpiritAuthored, SpiritAuthored, static_cast<uint8_t>(Type::WATER),
             SpiritAuthored, 52, 52);
    battle::BattleState &state =
        const_cast<battle::BattleState &>(battleSession().state());
    state.partyCount[0] = 3;
    state.bench[0][0].hp = 0; // KO'd original slot 1
    state.bench[0][1].hp = 10;
    BattleFlow::update(0);
    duelClearBuffer();
    renderBattle();

    test.expectEq(duelPixel(72, 21), 1, F("active member draws a pip"));
    test.expectEq(duelPixel(72, 25), 1, F("active member draws its underline"));
    test.expectEq(duelPixel(75, 21), 0, F("KO'd bench cell draws no pip"));
    test.expectEq(duelPixel(78, 21), 1, F("living bench cell draws its pip"));
}

// Zero-attack kit: the pick row collapses to SWAP + WAIT (two cells at x=4 and
// x=64), the WAIT cell is reachable, and the selected cell is a white box.
void duelDeviceWaitRow(FxTest &test) {
    duelPrep(SpiritAuthored, SpiritAuthored, static_cast<uint8_t>(Type::WATER),
             SpiritAuthored, 52, 52);
    battle::BattleState &state =
        const_cast<battle::BattleState &>(battleSession().state());
    state.partyCount[0] = 3;
    state.bench[0][0].hp = 10;
    state.bench[0][1].hp = 10;
    for (uint8_t slot = 0; slot < 4; ++slot) {
        state.active[0].moveTypes[slot] = EMPTY_MOVE_ID;
        state.active[0].moveIds[slot] = EMPTY_MOVE_ID;
    }
    BattleFlow::update(0);
    duelClearBuffer();
    renderBattle();

    // cell = 120/2 = 60: SWAP box at x=4, WAIT box at x=64.
    test.expectEq(duelAnyPixel(4, 61, 48, 61), 1, F("SWAP cell draws its label"));
    test.expectEq(duelAnyPixel(64, 121, 48, 61), 1, F("WAIT cell draws its label"));
    test.expectEq(duelAnyPixel(122, 127, 48, 61), 0, F("WAIT row stays inside 122px"));
}

// An exhausted player party fails real setup. The duel must terminate at entry
// as a loss through the normal Flow defeat recovery: no opponent action, no
// reward, and the world resumes. Drives the resident battleSession() so the real
// Flow path (not just the session) is exercised.
__attribute__((noinline)) void duelDeviceExhaustedEntryLoses(FxTest &test) {
    preparePlayer();
    for (uint8_t slot = 0; slot < PARTY_SIZE; ++slot) player.creatureHPs[slot] = 0;

    menu.clear();
    dialogMenu.clear();
    enterBattle();
    gameState.state = GameState_t::BATTLE;
    battleSession().setRng({scriptedRng});
    battleSession().beginWild(4, 1, false, 0);

    test.expectEq(battleSession().isActive(), 1, F("exhausted entry stays active"));
    test.expectEq(battleSession().awaitingPlayer(), 0, F("exhausted entry takes no input"));
    test.expectEq(battleSession().exitReady(), 1, F("exhausted entry is terminal"));

    // The terminal frame finishes the battle: defeat recovery returns to world.
    test.expectEq(BattleFlow::update(0), 1, F("the terminal frame finishes the flow"));
    test.expectEq(static_cast<uint32_t>(gameState.state),
                  static_cast<uint32_t>(GameState_t::WORLD),
                  F("defeat recovery returns to the world"));
}

// Render immediately after World->BATTLE, before the first Flow update closes
// the terminal. A drained active must not paint a species-0 sprite or a type-255
// badge; species 0 at a real level still draws (other tests cover that).
__attribute__((noinline)) void duelDeviceEmptyEntryRender(FxTest &test) {
    preparePlayer();
    for (uint8_t slot = 0; slot < PARTY_SIZE; ++slot) player.creatureHPs[slot] = 0;

    menu.clear();
    dialogMenu.clear();
    enterBattle();
    gameState.state = GameState_t::BATTLE;
    battleSession().setRng({scriptedRng});
    battleSession().beginWild(4, 1, false, 0);
    test.expectEq(battleSession().exitReady(), 1, F("exhausted entry is terminal"));

    // Render before any Flow update (the first BATTLE frame's scene draw).
    duelClearBuffer();
    renderBattle();
    test.expectEq(duelAnyPixel(0, 47, 0, 47), 0,
                  F("empty foe slot leaves its sprite area black"));
    test.expectEq(duelAnyPixel(80, 127, 0, 47), 0,
                  F("empty player slot leaves its sprite area black"));
}

} // namespace duel_ui_test_detail

inline void test_duel_ui(FxTest &test) {
    using namespace duel_ui_test_detail;
    duelDeviceGatherAcquisition(test);
    duelDeviceFleeCaption(test);
    duelDeviceBadgeAndHp(test);
    duelDeviceDualBadges(test);
    duelDeviceKoPips(test);
    duelDeviceWaitRow(test);
    duelDeviceThreeDigitHp(test);
    duelDevicePickRowGeometry(test);
    duelDeviceFeedback(test);
    duelDeviceExhaustedEntryLoses(test);
    duelDeviceEmptyEntryRender(test);
}

#endif // CGFX_DUEL_BATTLE
