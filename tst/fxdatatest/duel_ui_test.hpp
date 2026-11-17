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
#include "src/engine/battle/BattleFlow.hpp"
#include "src/engine/battle/BattlePresets.hpp"
#include "src/engine/battle/BattleScene.hpp"
#include "src/engine/battle/BattleSession.hpp"
#include "src/engine/ModeState.hpp"
#include "src/engine/game/Gamestate.hpp"
#include "src/engine/menu/MenuV2.hpp"
#include "src/item/Inventory.hpp"
#include "src/lib/FxReadCounter.hpp"
#include "src/player/Player.hpp"

namespace duel_ui_test_detail {

// Self-contained framebuffer fixtures. The boss-focused device session header
// owns only its own proof, so this suite carries the framing helpers it needs.

const uint8_t SpiritAuthored = static_cast<uint8_t>(Type::SPIRIT);
const uint8_t FireAuthored = static_cast<uint8_t>(Type::FIRE);

uint8_t rngDraws = 0;

uint8_t scriptedRng(uint8_t bound) {
    ++rngDraws;
    return bound == 0 ? 0 : static_cast<uint8_t>((rngDraws - 1) % bound);
}

void preparePlayer() {
    battle::applyPlayerPreset(player, BattlePresets::opening);
}

void beginWildDuel(bool gatherable, uint8_t tierRate) {
    menu.clear();
    dialogMenu.clear();
    enterBattle();
    gameState.state = GameState_t::BATTLE;
    rngDraws = 0;
    battleSession().setRng({scriptedRng});
    FxReadCounter::resetFrame();
    battleSession().beginWild(4, 1, gatherable, tierRate);
}

// Known matchup on the resident session; only the packed move TYPE is retyped.
__attribute__((noinline)) void configureMatchup(uint8_t playerType, uint8_t playerMove,
                                                uint8_t opponentType, uint8_t opponentMove,
                                                uint8_t playerHp, uint8_t opponentHp) {
    battle::BattleState &state =
        const_cast<battle::BattleState &>(battleSession().state());
    battle::Combatant &hero = state.active[static_cast<uint8_t>(battle::Side::Player)];
    hero.types = DualType(static_cast<Type>(playerType));
    hero.hp = hero.maxHp = playerHp;
    hero.attackMask = static_cast<uint8_t>(1u << duel::foldType(playerMove));

    battle::Combatant &foe = state.active[static_cast<uint8_t>(battle::Side::Opponent)];
    foe.types = DualType(static_cast<Type>(opponentType));
    foe.hp = foe.maxHp = opponentHp;
    if (foe.attackMask != 0)
        foe.attackMask = static_cast<uint8_t>(1u << duel::foldType(opponentMove));
}

bool runToResult(battle::BattleSession &session, uint16_t &ticks) {
    for (ticks = 0; ticks < 400; ++ticks) {
        if (session.isResultDwell()) return true;
        session.advance();
    }
    return session.isResultDwell();
}

// Mirror the production BATTLE draw dispatch: the scene owns the pick row,
// centre and footer; the menu paints only its own pages.
__attribute__((noinline)) void renderBattle() {
    battle::BattleScene::draw(battleSession().state(),
                              modeState.battle.ui.menu.duelCursor());
    if (battleSession().state().feedback == 0 &&
        modeState.battle.ui.menu.initialized())
        modeState.battle.ui.menu.draw();
}

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

// Six-cell worst case + centre strip geometry: five attacks + SWAP, no overlap.
void duelDevicePickRowGeometry(FxTest &test) {
    duelPrep(SpiritAuthored, SpiritAuthored, static_cast<uint8_t>(Type::WATER),
             SpiritAuthored, 52, 52);
    battle::BattleState &state =
        const_cast<battle::BattleState &>(battleSession().state());
    state.partyCount[0] = 3;
    state.bench[0][0].hp = 10;
    state.bench[0][1].hp = 10;
    // All five set mask bits -> five attack cells plus the SWAP cell.
    state.active[0].attackMask = 0x1F;
    BattleFlow::update(0); // choice entry -> pick row
    duelClearBuffer();
    renderBattle();

    // Five attacks + SWAP = six cells: cell = 20, boxes at x = 4, 24, 44, 64, 84,
    // 104 with an 18px white box, so adjacent boxes keep a 2px black gap.
    test.expectEq(duelPixel(5, 49), 1, F("selected first cell is a white box"));
    test.expectEq(duelPixel(22, 49), 0, F("gap between cell 0 and cell 1 stays black"));
    test.expectEq(duelPixel(42, 49), 0, F("gap between cell 1 and cell 2 stays black"));
    test.expectEq(duelPixel(62, 49), 0, F("gap between cell 2 and cell 3 stays black"));
    test.expectEq(duelPixel(82, 49), 0, F("gap between cell 3 and cell 4 stays black"));
    test.expectEq(duelPixel(102, 49), 0, F("gap between cell 4 and SWAP stays black"));
    test.expectEq(duelAnyPixel(104, 121, 48, 61), 1, F("SWAP cell draws its label"));
    test.expectEq(duelAnyPixel(122, 127, 48, 61), 0, F("row stays inside 122px"));
    // Centre strip stays within x48..79; the selected icon ends by x65.
    test.expectEq(duelAnyPixel(48, 56, 41, 45), 1, F("centre R# is drawn"));
    test.expectEq(duelAnyPixel(57, 64, 40, 47), 1, F("centre selected type icon drawn"));
    test.expectEq(duelAnyPixel(68, 79, 41, 47), 0,
                  F("ordinary wild centre stays left of the sprite"));
}

// The persistent GATHER cue: present iff the encounter is a gatherable wild,
// replacing the round/type content at x48/y41 and keeping y47 clear.
__attribute__((noinline)) void duelDeviceGatherCue(FxTest &test) {
    duelPrep(SpiritAuthored, SpiritAuthored, static_cast<uint8_t>(Type::WATER),
             SpiritAuthored, 52, 52);
    battle::BattleState &state =
        const_cast<battle::BattleState &>(battleSession().state());
    state.partyCount[0] = 3;
    state.bench[0][0].hp = 10;
    state.bench[0][1].hp = 10;
    state.active[0].attackMask = 0x1F; // six-cell worst case
    BattleFlow::update(0);
    duelClearBuffer();
    renderBattle();

    // Ordinary wild: round/type content only, no cue tail.
    test.expectEq(duelAnyPixel(68, 77, 41, 46), 0,
                  F("an ordinary wild shows no gather cue"));

    // Same geometry in a real gatherable wild: the cue replaces the content.
    beginWildDuel(true, 0);
    battle::BattleState &gatherable =
        const_cast<battle::BattleState &>(battleSession().state());
    gatherable.partyCount[0] = 3;
    gatherable.bench[0][0].hp = 10;
    gatherable.bench[0][1].hp = 10;
    gatherable.active[0].attackMask = 0x1F; // six-cell worst case
    BattleFlow::update(0);
    duelClearBuffer();
    renderBattle();
    test.expectEq(duelAnyPixel(48, 77, 41, 46), 1,
                  F("a gatherable wild draws the centre cue"));
    test.expectEq(duelAnyPixel(68, 77, 41, 46), 1,
                  F("the cue replaces the round/type content"));
    test.expectEq(duelAnyPixel(48, 79, 47, 47), 0,
                  F("the cue leaves the progress row clear"));
    test.expectEq(duelAnyPixel(78, 79, 40, 47), 0,
                  F("the cue stays left of the player sprite"));
    // The six-cell row is unaffected by the cue.
    test.expectEq(duelPixel(5, 49), 1, F("cue keeps the pick row's selected cell"));
    test.expectEq(duelAnyPixel(104, 121, 48, 61), 1, F("cue keeps the SWAP cell"));
    test.expectEq(duelAnyPixel(122, 127, 48, 61), 0, F("cue keeps the row inside 122px"));
}

// The B action page stays the selector: opening it and committing the first row
// issues the existing Gather intent and banks tier progress.
__attribute__((noinline)) void duelDeviceGatherActionPage(FxTest &test) {
    preparePlayer();
    beginWildDuel(true, 1); // gatherable wild, tierRate 1 (need is 8 at level 1)
    configureMatchup(SpiritAuthored, SpiritAuthored, FireAuthored, SpiritAuthored, 52, 52);

    test.expectEq(BattleFlow::update(0), 0, F("choice frame stays in battle"));
    test.expectEq(BattleFlow::update(MENU_EDGE_B), 0,
                  F("B opens the action page without resolving"));
    test.expectEq(BattleFlow::update(MENU_EDGE_A), 0,
                  F("A on the first action row commits gather"));
    test.expectEq(battleSession().isResultDwell(), 0,
                  F("the gather commit starts the concealed phase"));
    test.expectEq(battleSession().state().gather.progress, 0,
                  F("progress waits for the reveal"));

    uint16_t frames = 0;
    while (!battleSession().isResultDwell() && frames < 400) {
        BattleFlow::update(0);
        ++frames;
    }
    test.expectEq(battleSession().isResultDwell(), 1, F("the gather round reveals"));
    test.expectEq(battleSession().state().feedback,
                  static_cast<uint8_t>(duel::Feedback::Gather),
                  F("the reveal is a gather round, not an attack"));
    test.expectEq(battleSession().state().gather.progress, 1,
                  F("the action page issued the Gather intent"));
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
    state.active[0].attackMask = 0; // no usable attack -> SWAP + WAIT
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

// The duel SWAP bench picker shares the pick row's convention: a BLACK strip
// whose selected row is a WHITE cell (dark text) with white text on the other
// row. Fails on the pre-fix inverted strip (white band, black selected row).
__attribute__((noinline)) void duelDevicePartyStripColors(FxTest &test) {
    preparePlayer();
    beginWildDuel(false, 0);
    configureMatchup(SpiritAuthored, SpiritAuthored, FireAuthored, SpiritAuthored, 52, 52);
    battle::BattleState &state =
        const_cast<battle::BattleState &>(battleSession().state());
    // Two living bench members -> the SWAP cell is offered and both picker rows
    // cache a real creature name.
    state.partyCount[0] = 3;
    state.bench[0][0].id = player.party[1].id;
    state.bench[0][0].level = player.party[1].level;
    state.bench[0][0].hp = 10;
    state.bench[0][1].id = player.party[2].id;
    state.bench[0][1].level = player.party[2].level;
    state.bench[0][1].hp = 10;

    BattleFlow::update(0);              // choice entry -> pick row
    BattleFlow::update(MENU_NAV_RIGHT); // cursor -> SWAP cell (one attack + SWAP)
    BattleFlow::update(MENU_EDGE_A);    // SWAP -> the two-row bench picker
    duelClearBuffer();
    renderBattle();

    // x=100 is clear of the names (they start at x=4); x=2 proves the full width.
    test.expectEq(duelPixel(100, 50), 1, F("picker selected row is a white cell"));
    test.expectEq(duelPixel(2, 50), 1, F("picker selected cell spans the row width"));
    test.expectEq(duelPixel(100, 58), 0, F("picker strip stays black below the selection"));
    test.expectEq(duelPixel(2, 58), 0, F("unselected picker row stays on the black strip"));
}

// The wild action page (GATHER/ITEMS/ESCAPE) matches the same convention as
// the pick row: a BLACK strip with a WHITE selected cell. The three-row page
// narrows to the left column so the player sprite stays visible.
__attribute__((noinline)) void duelDeviceActionStripColors(FxTest &test) {
    preparePlayer();
    beginWildDuel(true, 1); // gatherable wild -> GATHER + ITEMS + ESCAPE rows
    configureMatchup(SpiritAuthored, SpiritAuthored, FireAuthored, SpiritAuthored, 52, 52);

    BattleFlow::update(0);           // choice entry -> pick row
    BattleFlow::update(MENU_EDGE_B); // open the action page (cursor on GATHER)
    duelClearBuffer();
    renderBattle();

    test.expectEq(duelPixel(2, 42), 1, F("action selected row is a white cell"));
    test.expectEq(duelPixel(70, 42), 1, F("action selected cell spans the left column"));
    test.expectEq(duelPixel(2, 50), 0, F("the ITEMS row stays black"));
    test.expectEq(duelPixel(2, 58), 0, F("the ESCAPE row stays black"));
}

// jp8.5.19: the ITEMS row opens the cached four-row Heal bag over the duel and
// the confirmed selection heals once through the real Flow.
__attribute__((noinline)) void duelDeviceItemPage(FxTest &test) {
    preparePlayer();
    item::inventoryClear(player.inventory);
    item::inventoryAdd(player.inventory, item::ItemKind::Consumable, 0, 1);
    item::setBattleInventory(&player.inventory);
    beginWildDuel(false, 0); // non-gatherable -> ITEMS first on the B page
    configureMatchup(FireAuthored, FireAuthored, SpiritAuthored, SpiritAuthored,
                     100, 100);
    battle::BattleState &state =
        const_cast<battle::BattleState &>(battleSession().state());
    state.active[0].maxHp = 30;
    state.active[0].hp = 20;

    BattleFlow::update(0);           // choice entry
    BattleFlow::update(MENU_EDGE_B); // action page (ITEMS first)
    FxReadCounter::resetFrame();
    test.expectEq(BattleFlow::update(MENU_EDGE_A), 0,
                  F("A opens the bag without an action"));
    test.expectEq(FxReadCounter::markUpdate(), 1,
                  F("bag-open reads are transition-only"));
    test.expectEq(item::inventoryCount(player.inventory,
                                       item::ItemKind::Consumable, 0),
                  static_cast<uint8_t>(1), F("opening the bag takes nothing"));
    duelClearBuffer();
    renderBattle();
    test.expectEq(duelAnyPixel(17, 48, 8, 14), 1, F("the tonic row is drawn"));

    test.expectEq(BattleFlow::update(MENU_EDGE_A), 0,
                  F("A confirms the cached tonic"));
    test.expectEq(item::inventoryCount(player.inventory,
                                       item::ItemKind::Consumable, 0),
                  static_cast<uint8_t>(0), F("the tonic is taken once"));
    uint16_t frames = 0;
    while (!battleSession().isResultDwell() && frames < 400) {
        BattleFlow::update(0);
        ++frames;
    }
    test.expectEq(battleSession().isResultDwell(), 1, F("the item round reveals"));
    test.expectEq(state.active[0].hp, static_cast<uint8_t>(9),
                  F("the clamped heal lands before the committed hit"));
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
    duelDeviceGatherCue(test);
    duelDeviceGatherActionPage(test);
    duelDevicePartyStripColors(test);
    duelDeviceActionStripColors(test);
    duelDeviceItemPage(test);
    duelDeviceFeedback(test);
    duelDeviceExhaustedEntryLoses(test);
    duelDeviceEmptyEntryRender(test);
}

#endif // CGFX_DUEL_BATTLE
