#pragma once
// Real-session regression coverage for the RPS duel battle
// (CreatureGathererFX-d73i.5, spike step 1).
//
// These suites drive the ACTUAL battle::BattleSession shipped by the duel build:
// the Choice-entry concealed commitment, the 29 Concealed frame updates, the 68
// Result dwell frames, the one atomic simultaneous round, original-slot HP
// ownership, WAIT and the gather/escape ordering. The pure kernel relations and
// personas stay in tst/duel_kernel_test.hpp; this file proves the session/flow
// wiring around them.
//
// Compiled only in duel host runs: `make test CPPFLAGS='-I. -DCGFX_DUEL_BATTLE'`
// (registered from tst/main.cpp under the same macro).
//
// Fixtures are built from authored BattlePresets kits (applyPlayerPreset) and
// their real loaded Move/id pairs; only the packed move TYPE is retyped, which
// the plan allows as a demo fixture. No literal record indices are invented and
// no forged empty/status intents are sent — the normal menu route supplies Pass.

#ifdef CGFX_DUEL_BATTLE

#include <string.h>
#include "test.hpp"
#include "src/FXDataFake.hpp"
#include "../src/engine/battle/DuelView.hpp"
#include "../src/engine/battle/BattleSession.hpp"
#include "../src/engine/battle/BattleFlow.hpp"
#include "../src/engine/battle/BattlePresets.hpp"
#include "../src/engine/ModeState.hpp"
#include "../src/engine/game/Gamestate.hpp"
#include "../src/engine/menu/MenuV2.hpp"
#include "../src/lib/MoveIds.hpp"
#include "../src/lib/FxReadCounter.hpp"
#include "../src/player/Player.hpp"
#include "../src/globals.hpp"
#include "../src/save/Compaction.hpp"
#include "../src/save/FlashBackend.hpp"
#include "../src/creature/LevelCurve.hpp"
#include "../src/lib/ReadData.hpp"
#include "../src/fxdata.h"

namespace duel_session_test_detail {

// Frozen duel timing owned by BattleSession (see its DUEL_*_TICKS).
constexpr uint16_t kConcealTicks = 29;
constexpr uint16_t kResultTicks = 68;

uint8_t rngDraws = 0;

uint8_t scriptedRng(uint8_t bound) {
    ++rngDraws;
    return bound == 0 ? 0 : static_cast<uint8_t>((rngDraws - 1) % bound);
}

// Authored 8-type ids; the session folds SPIRIT->Spirit, FIRE->Fire.
const uint8_t SpiritAuthored = static_cast<uint8_t>(Type::SPIRIT);
const uint8_t FireAuthored = static_cast<uint8_t>(Type::FIRE);

// Three authored species with full persistent HP, matching a real party that
// enters an encounter. Party ids/levels/moves stay real for every later switch.
void preparePlayer() {
    player = Player();
    battle::applyPlayerPreset(player, BattlePresets::opening);
}

// Fully known matchup on a session. The MEANINGFUL player slot keeps its
// authored move id; only its packed type changes (retype fixture). The foe's
// authored-usable slots are all retyped to one type, so whichever option the
// Choice-entry commit picked maps here. No record indices are invented.
void configureMatchup(battle::BattleSession &session, uint8_t playerType,
                      uint8_t playerMove, uint8_t opponentType, uint8_t opponentMove,
                      uint8_t playerHp, uint8_t opponentHp) {
    battle::BattleState &state = session.stateForTest();
    battle::Combatant &hero = state.active[static_cast<uint8_t>(battle::Side::Player)];
    hero.types = DualType(static_cast<Type>(playerType));
    hero.hp = hero.maxHp = playerHp;
    // One attack option: its folded type is option 0 (the lowest set bit).
    hero.attackMask = static_cast<uint8_t>(1u << duel::foldType(playerMove));

    battle::Combatant &foe = state.active[static_cast<uint8_t>(battle::Side::Opponent)];
    foe.types = DualType(static_cast<Type>(opponentType));
    foe.hp = foe.maxHp = opponentHp;
    // Collapse the foe's authored kit to one type so whichever option the
    // Choice-entry commit picked maps here; an unusable kit stays WAIT.
    if (foe.attackMask != 0)
        foe.attackMask = static_cast<uint8_t>(1u << duel::foldType(opponentMove));
}

// Advance through the Concealed phase to the reveal (Result phase begins).
bool runToResult(battle::BattleSession &session, uint16_t &ticks) {
    for (ticks = 0; ticks < 400; ++ticks) {
        if (session.isResultDwell()) return true;
        session.advance();
    }
    return session.isResultDwell();
}

// Advance the Result dwell to its boundary (next Choice or Terminal).
bool runToBoundary(battle::BattleSession &session, uint16_t &ticks) {
    for (ticks = 0; ticks < 400; ++ticks) {
        if (!session.isResultDwell()) return true;
        session.advance();
    }
    return !session.isResultDwell();
}

// The concealed opponent pick commits once on entry to Choice, before the
// player's input; navigation consumes no RNG and a submission never rerolls.
void duelCommitBeforeSubmission(Test &test) {
    preparePlayer();
    battle::BattleSession session;
    session.setRng({scriptedRng});
    rngDraws = 0;
    session.beginWild(4, 1, true, 1);

    test.assert(session.isActive(), true, "wild duel session is active");
    test.assert(session.awaitingPlayer(), true, "wild duel opens at a choice");
    test.assert(rngDraws, static_cast<uint8_t>(1),
                "choice entry commits exactly one bounded draw");
    test.assert(session.submitIntent({MenuIntentKind::None, 0}), false,
                "None is never a battle action");
    test.assert(session.submitIntent({MenuIntentKind::Back, 0}), false,
                "Back is never a battle action");
    test.assert(rngDraws, static_cast<uint8_t>(1),
                "navigation commits no new hidden RNG");
    test.assert(session.submitIntent({MenuIntentKind::Gather, 0}), true,
                "a wild gather submits");
    test.assert(rngDraws, static_cast<uint8_t>(1),
                "submission does not reroll the concealed pick");
    test.assert(session.isResultDwell(), false,
                "accepting a pick opens the concealed interval");
}

// End-to-end flow timing: exactly 29 concealed frame updates before the reveal,
// a held edge cannot skip them, no HP changes before the reveal, and exactly 68
// result dwell updates before a fresh choice.
void duelConcealAndDwellFlow(Test &test) {
    preparePlayer();
    menu.clear();
    dialogMenu.clear();
    enterBattle();
    gameState.state = GameState_t::BATTLE;
    rngDraws = 0;
    battleSession().setRng({scriptedRng});
    battleSession().beginWild(4, 1, false, 0);
    configureMatchup(battleSession(), FireAuthored, SpiritAuthored, FireAuthored,
                     FireAuthored, 100, 100);

    test.assert(BattleFlow::update(0), false, "choice frame stays in battle");
    test.assert(modeState.battle.ui.menu.initialized(), true, "pick row opens");
    BattleFlow::update(MENU_EDGE_A); // pick row: commit the selected attack cell
    test.assert(battleSession().isResultDwell(), false,
                "selection never resolves on its own edge");
    test.assert(battleSession().result().kind, battle::ResultKind::None,
                "no result is published before the reveal");
    test.assert(rngDraws, static_cast<uint8_t>(1),
                "the committed pick is not rerolled by the flow");

    const uint8_t playerHp = battleSession().state().active[0].hp;
    const uint8_t opponentHp = battleSession().state().active[1].hp;
    uint16_t conceal = 0;
    while (!battleSession().isResultDwell() && conceal < 200) {
        test.assert(battleSession().state().active[0].hp, playerHp,
                    "no player damage before the reveal");
        test.assert(battleSession().state().active[1].hp, opponentHp,
                    "no foe damage before the reveal");
        BattleFlow::update(MENU_EDGE_A); // a held edge must not skip the interval
        ++conceal;
    }
    test.assert(conceal, kConcealTicks, "exactly 29 concealed frame updates");
    test.assert(battleSession().isResultDwell(), true, "reveal begins the result dwell");
    test.assert(battleSession().result().kind, battle::ResultKind::Attack,
                "the reveal publishes the round");
    test.assert(battleSession().state().active[1].hp, static_cast<uint8_t>(79),
                "winning hit lands 21 at the reveal");
    test.assert(battleSession().state().active[0].hp, static_cast<uint8_t>(96),
                "chip lands 4 at the reveal");

    uint16_t dwell = 0;
    bool dwellReads = true;
    bool boundaryConsumed = false;
    while (battleSession().isResultDwell() && dwell < 400) {
        FxReadCounter::resetFrame();
        BattleFlow::update(0);
        if (battleSession().isResultDwell() &&
            gameState.state == GameState_t::BATTLE) {
            // A steady result frame (still Result after the update) performs no
            // metadata reads; captions were prepared on the reveal transition.
            if (FxReadCounter::count() != 0) dwellReads = false;
        } else {
            // The 68th update transitions to Choice and legitimately reads
            // move-name metadata to build the next menu. It must also have
            // consumed the choice UI entry on that same frame.
            boundaryConsumed = !battleSession().consumeChoiceEntry();
        }
        ++dwell;
    }
    test.assert(dwell, kResultTicks, "exactly 68 result frame updates");
    test.assert(dwellReads, true, "steady result frames stay metadata-free");
    test.assert(boundaryConsumed, true, "the boundary frame consumes the choice entry");
    test.assert(battleSession().awaitingPlayer(), true,
                "the round returns to a fresh choice");
    exitBattle();
    gameState.state = GameState_t::WORLD;
}

// Transition-entry ownership: the flow must consume the result entry on the
// same frame the reveal happens (so the feedback union arm is built before that
// frame's draw) and the choice entry on the same frame the dwell boundary opens
// the next choice. Correct code leaves both flags cleared at the boundary.
void duelEntryFlagsConsumedSameFrame(Test &test) {
    preparePlayer();
    menu.clear();
    dialogMenu.clear();
    enterBattle();
    gameState.state = GameState_t::BATTLE;
    rngDraws = 0;
    battleSession().setRng({scriptedRng});
    battleSession().beginWild(4, 1, false, 0);
    configureMatchup(battleSession(), FireAuthored, SpiritAuthored, FireAuthored,
                     FireAuthored, 100, 100);

    BattleFlow::update(0);            // choice entry -> pick row
    BattleFlow::update(MENU_EDGE_A);  // commit the selected attack cell
    uint16_t conceal = 0;
    while (!battleSession().isResultDwell() && conceal < 200) {
        BattleFlow::update(0);
        ++conceal;
    }
    test.assert(conceal, kConcealTicks, "the reveal is 29 updates later");
    test.assert(battleSession().consumeResultEntry(), false,
                "result entry is consumed on the reveal frame");

    uint16_t dwell = 0;
    while (battleSession().isResultDwell() && dwell < 400) {
        BattleFlow::update(0);
        ++dwell;
    }
    test.assert(dwell, kResultTicks, "the dwell is 68 updates");
    test.assert(battleSession().consumeChoiceEntry(), false,
                "choice entry is consumed on the boundary frame");
    test.assert(battleSession().awaitingPlayer(), true, "the next choice is open");
    exitBattle();
    gameState.state = GameState_t::WORLD;
}

// Immediate wild escape resolves straight into the Result phase; the flow must
// still build the feedback union arm on that same submit update.
void duelEscapeEntryConsumedSameFrame(Test &test) {
    preparePlayer();
    menu.clear();
    dialogMenu.clear();
    enterBattle();
    gameState.state = GameState_t::BATTLE;
    rngDraws = 0;
    battleSession().setRng({scriptedRng});
    battleSession().beginWild(4, 1, true, 1);

    BattleFlow::update(0);              // choice entry -> pick row
    BattleFlow::update(MENU_EDGE_B);    // open the wild action page
    BattleFlow::update(MENU_NAV_DOWN);  // cursor -> ESCAPE
    BattleFlow::update(MENU_EDGE_A);    // submit Escape -> immediate Result
    test.assert(battleSession().isResultDwell(), true,
                "escape enters the result dwell immediately");
    test.assert(battleSession().result().kind, battle::ResultKind::Escape,
                "the escape result is published");
    test.assert(battleSession().consumeResultEntry(), false,
                "escape constructs feedback on the same update");
    exitBattle();
    gameState.state = GameState_t::WORLD;
}

// One atomic round at the session level: concealed first, then a single shared
// result, with both hits applied against the same pre-hit state.
void duelAtomicAttackRound(Test &test) {
    preparePlayer();
    battle::BattleSession session;
    session.setRng({scriptedRng});
    rngDraws = 0;
    session.beginWild(4, 1, false, 0);
    configureMatchup(session, FireAuthored, SpiritAuthored, FireAuthored,
                     FireAuthored, 100, 100);

    test.assert(session.submitIntent({MenuIntentKind::SelectMove, 0}), true,
                "an authored attack slot submits");
    test.assert(session.isResultDwell(), false, "the round starts concealed");
    test.assert(session.result().kind, battle::ResultKind::None,
                "no result is published while concealed");

    uint16_t revealTicks = 0;
    test.assert(runToResult(session, revealTicks), true, "the concealed round reveals");
    test.assert(revealTicks, kConcealTicks, "the reveal is 29 updates later");
    test.assert(session.result().kind, battle::ResultKind::Attack,
                "the revealed result is the shared attack");
    test.assert(session.state().active[0].hp, static_cast<uint8_t>(96),
                "chip keeper loses 4");
    test.assert(session.state().active[1].hp, static_cast<uint8_t>(79),
                "winning hit lands 21");

    uint16_t dwellTicks = 0;
    test.assert(runToBoundary(session, dwellTicks), true, "the result dwell settles");
    test.assert(dwellTicks, kResultTicks, "the dwell is 68 updates");
}

// Simultaneous KO is a Draw, terminal, with both faint flags and original-slot
// zeroing.
void duelSimultaneousDraw(Test &test) {
    preparePlayer();
    player.creatureHPs[1] = 0;
    player.creatureHPs[2] = 0;
    battle::BattleSession session;
    session.setRng({scriptedRng});
    rngDraws = 0;
    session.beginWild(4, 1, false, 0);
    // Mirrored Spirit trades 6/6, both into 5 HP.
    configureMatchup(session, SpiritAuthored, SpiritAuthored, SpiritAuthored,
                     SpiritAuthored, 5, 5);

    test.assert(session.submitIntent({MenuIntentKind::SelectMove, 0}), true,
                "the mirror trade submits");
    uint16_t revealTicks = 0;
    test.assert(runToResult(session, revealTicks), true, "the trade reveals");
    test.assert(session.state().active[0].hp, static_cast<uint8_t>(0),
                "player active faints");
    test.assert(session.state().active[1].hp, static_cast<uint8_t>(0),
                "opponent active faints");
    test.assert(session.result().outcome, battle::Outcome::Draw,
                "simultaneous KO is a draw");
    test.assert((session.result().flags & battle::PLAYER_FAINTED) != 0, true,
                "player faint is flagged on the shared result");
    test.assert((session.result().flags & battle::OPPONENT_FAINTED) != 0, true,
                "opponent faint is flagged on the shared result");

    uint16_t dwellTicks = 0;
    test.assert(runToBoundary(session, dwellTicks), true, "the draw dwell settles");
    test.assert(session.exitReady(), true, "a draw is terminal");
    test.assert(player.creatureHPs[0], static_cast<uint8_t>(0),
                "draw syncs the faint to its original slot");
    test.assert(session.state().feedback,
                static_cast<uint8_t>(duel::Feedback::Draw),
                "a simultaneous KO shows the DRAW caption");
}

// An unusable kit carries empty sentinels in the shipping move snapshot and its
// WAIT (Pass) route commits no attack and receives the opponent default win.
void duelWaitReceivesDefaultWin(Test &test) {
    preparePlayer();
    battle::BattleSession session;
    session.setRng({scriptedRng});
    rngDraws = 0;
    session.beginWild(4, 1, false, 0);

    battle::BattleState &state = session.stateForTest();
    state.active[0].attackMask = 0;
    state.active[0].types = DualType(static_cast<Type>(FireAuthored));
    state.active[0].hp = state.active[0].maxHp = 100;
    state.active[1].types = DualType(static_cast<Type>(FireAuthored));
    state.active[1].hp = state.active[1].maxHp = 100;
    state.active[1].attackMask = static_cast<uint8_t>(1u << duel::foldType(SpiritAuthored));

    test.assert(duel::usableOptionCount(state, 0), static_cast<uint8_t>(0),
                "an unusable kit has no attack options");
    const battle::MoveSnapshot snapshot = session.moves();
    bool allEmpty = true;
    for (uint8_t slot = 0; slot < 4; ++slot)
        if (snapshot.moveIds[slot] != EMPTY_MOVE_ID) allEmpty = false;
    test.assert(allEmpty, true, "an unusable kit exposes only empty cells");

    test.assert(session.submitIntent({MenuIntentKind::Pass}), true,
                "the WAIT route submits");
    uint16_t revealTicks = 0;
    test.assert(runToResult(session, revealTicks), true, "WAIT reveals");
    // Spirit beats Fire -> 14 x1.5 = 21 into the waiting side, zero back.
    test.assert(session.state().active[0].hp, static_cast<uint8_t>(79),
                "the waiter takes the default win");
    test.assert(session.state().active[1].hp, static_cast<uint8_t>(100),
                "WAIT deals no damage");
}

// A voluntary swap that gets the incoming member KO'd must restore the outgoing
// original slot (2) when it still lives — not the first living slot (1).
void duelSwapKoRevertsToOutgoing(Test &test) {
    preparePlayer();
    battle::BattleSession session;
    session.setRng({scriptedRng});
    rngDraws = 0;
    session.beginWild(4, 1, false, 0);

    battle::BattleState &state = session.stateForTest();
    // Physically real post-switch shape: a 0 -> 2 switch leaves original slot 2
    // active with originals 0 and 1 living on the bench in party order. The
    // cached incoming creature keeps its authored id/level; only its type is
    // retyped (demo retype fixture).
    const uint8_t slot0Id = player.party[0].id;
    const uint8_t slot0Level = player.party[0].level;
    player.party[0].types = DualType(static_cast<Type>(FireAuthored));

    state.partyCount[0] = 3;
    state.activeSlot[0] = 2;
    state.active[0].id = player.party[2].id;
    state.active[0].level = player.party[2].level;
    state.active[0].types = DualType(static_cast<Type>(SpiritAuthored));
    state.active[0].hp = state.active[0].maxHp = 50;

    state.bench[0][0].id = slot0Id;
    state.bench[0][0].level = slot0Level;
    state.bench[0][0].hp = 10; // incoming: dies to the default win
    state.bench[0][1].id = player.party[1].id;
    state.bench[0][1].level = player.party[1].level;
    state.bench[0][1].hp = 50; // living first-live slot that must NOT be chosen

    state.activeSlot[1] = 0;
    state.active[1].types = DualType(static_cast<Type>(SpiritAuthored));
    state.active[1].hp = state.active[1].maxHp = 100;
    state.active[1].attackMask = static_cast<uint8_t>(1u << duel::foldType(SpiritAuthored));

    test.assert(session.submitIntent({MenuIntentKind::SelectParty, 0}), true,
                "the living incoming slot is selectable");
    uint16_t revealTicks = 0;
    test.assert(runToResult(session, revealTicks), true, "the swap round reveals");
    test.assert(state.activeSlot[0], static_cast<uint8_t>(2),
                "incoming KO restores the outgoing original slot");
    test.assert(state.active[0].id, player.party[2].id,
                "the restored member is the outgoing creature");
    test.assert(state.active[0].hp, static_cast<uint8_t>(50),
                "the restored member keeps its original-slot HP");
}

// An ordinary KO promotes the lowest living original slot.
void duelOrdinaryKoPromotesFirstLiving(Test &test) {
    preparePlayer();
    battle::BattleSession session;
    session.setRng({scriptedRng});
    rngDraws = 0;
    session.beginWild(4, 1, false, 0);
    configureMatchup(session, FireAuthored, FireAuthored, SpiritAuthored,
                     SpiritAuthored, 5, 100);
    test.assert(session.state().partyCount[0], static_cast<uint8_t>(3),
                "three player originals loaded");
    test.assert(session.submitIntent({MenuIntentKind::SelectMove, 0}), true,
                "an authored attack submits");
    uint16_t revealTicks = 0;
    test.assert(runToResult(session, revealTicks), true, "the knockout reveals");
    test.assert(session.state().activeSlot[0], static_cast<uint8_t>(1),
                "ordinary KO promotes the first living original slot");
    test.assert(session.state().active[0].hp != 0, true,
                "the promoted member is alive");
}

// Gather adds tier progress before the opponent's default win; a KO this round
// blocks acquisition but never discards the added progress.
void duelGatherKoKeepsProgress(Test &test) {
    preparePlayer();
    player.creatureHPs[1] = 0;
    player.creatureHPs[2] = 0;
    battle::BattleSession session;
    session.setRng({scriptedRng});
    rngDraws = 0;
    session.beginWild(4, 1, true, 8); // gatherable, need 8, tierRate 8
    configureMatchup(session, FireAuthored, FireAuthored, FireAuthored,
                     SpiritAuthored, 5, 100);

    test.assert(session.submitIntent({MenuIntentKind::Gather, 0}), true,
                "a wild gather submits");
    uint16_t revealTicks = 0;
    test.assert(runToResult(session, revealTicks), true, "the gather round reveals");
    test.assert(session.result().kind, battle::ResultKind::Gather,
                "the revealed result is the gather");
    test.assert(session.state().gather.progress, static_cast<uint8_t>(8),
                "progress is banked before the hit");
    test.assert(session.result().outcome, battle::Outcome::Lose,
                "the knockout blocks acquisition");
    test.assert((session.result().flags & battle::PLAYER_FAINTED) != 0, true,
                "the gathering active is flagged faint");
    test.assert(session.state().feedback,
                static_cast<uint8_t>(duel::Feedback::Lose),
                "a terminal gather KO shows the defeat caption");
}

// A wild gather that KOs the active while a living reserve stands in must not
// acquire and must not run the flee countdown: the round pauses it on the
// active KO (the archived resolver waited for the replacement to act).
void duelGatherActiveKoPausesFleeCountdown(Test &test) {
    preparePlayer();
    player.creatureHPs[0] = 1;
    player.creatureHPs[1] = 50;
    player.creatureHPs[2] = 0;
    battle::BattleSession session;
    session.setRng({scriptedRng});
    rngDraws = 0;
    session.beginWild(4, 1, true, 1); // need 8, tierRate 1
    battle::BattleState &state = session.stateForTest();
    state.gather.progress = 5;
    state.gather.fleeTurns = 1;
    configureMatchup(session, FireAuthored, FireAuthored, FireAuthored,
                     SpiritAuthored, 1, 100);

    test.assert(session.state().active[0].hp, static_cast<uint8_t>(1),
                "active enters on one HP");
    test.assert(session.submitIntent({MenuIntentKind::Gather, 0}), true,
                "a wild gather submits");
    uint16_t revealTicks = 0;
    test.assert(runToResult(session, revealTicks), true, "the gather round reveals");
    test.assert(session.state().gather.progress, static_cast<uint8_t>(6),
                "progress is banked before the hit");
    test.assert(session.result().outcome, battle::Outcome::None,
                "an active KO neither acquires nor flees while a reserve lives");
    test.assert(session.state().gather.fleeTurns, static_cast<uint8_t>(1),
                "the flee countdown pauses on an active KO");
    test.assert(session.state().activeSlot[0], static_cast<uint8_t>(1),
                "the living reserve promotes");
    test.assert(session.state().feedback,
                static_cast<uint8_t>(duel::Feedback::Gather),
                "a progress-only gather keeps the gather caption");
}

// A surviving gather round banks progress and acquires.
void duelGatherAcquisition(Test &test) {
    preparePlayer();
    battle::BattleSession session;
    session.setRng({scriptedRng});
    rngDraws = 0;
    session.beginWild(4, 1, true, 8);
    configureMatchup(session, SpiritAuthored, SpiritAuthored, FireAuthored,
                     SpiritAuthored, 100, 100);

    test.assert(session.submitIntent({MenuIntentKind::Gather, 0}), true,
                "a wild gather submits");
    uint16_t revealTicks = 0;
    test.assert(runToResult(session, revealTicks), true, "the surviving gather reveals");
    test.assert(session.state().gather.progress, static_cast<uint8_t>(8),
                "progress reaches the need");
    test.assert(session.result().outcome, battle::Outcome::Gathered,
                "a surviving full gather is acquired");
    test.assert(session.state().over, true, "acquisition closes the encounter");
    test.assert(session.state().feedback,
                static_cast<uint8_t>(duel::Feedback::Gathered),
                "the footer shows the gathered caption on acquisition");
}

// Wild escape is immediate (no concealed interval) and cancels the round.
void duelEscapeCancelsRound(Test &test) {
    preparePlayer();
    battle::BattleSession session;
    session.setRng({scriptedRng});
    rngDraws = 0;
    session.beginWild(4, 1, true, 8);
    const uint8_t playerHp = session.state().active[0].hp;
    const uint8_t opponentHp = session.state().active[1].hp;

    test.assert(session.submitIntent({MenuIntentKind::Escape, 0}), true,
                "wild escape submits");
    test.assert(session.isResultDwell(), true,
                "escape resolves immediately into the result dwell");
    test.assert(session.result().kind, battle::ResultKind::Escape,
                "the result is the escape");
    test.assert(session.result().outcome, battle::Outcome::Escaped,
                "escape is terminal");
    test.assert(session.state().over, true, "escape closes the encounter");
    test.assert(session.state().active[0].hp, playerHp,
                "escape cancels the pending round (player HP unchanged)");
    test.assert(session.state().active[1].hp, opponentHp,
                "escape cancels the pending round (opponent HP unchanged)");
    test.assert(session.state().feedback,
                static_cast<uint8_t>(duel::Feedback::Escape),
                "the footer shows the escaped caption");

    uint16_t dwellTicks = 0;
    test.assert(runToBoundary(session, dwellTicks), true, "the escape dwell settles");
    test.assert(dwellTicks, kResultTicks, "the terminal dwell is 68 updates");
    test.assert(session.exitReady(), true, "escape is terminal after the dwell");
}

// The terminal sync writes every participating original slot back to the
// persistent roster, not just the active member.
void duelOriginalSlotHpSync(Test &test) {
    preparePlayer();
    battle::BattleSession session;
    session.setRng({scriptedRng});
    rngDraws = 0;
    session.beginWild(4, 1, false, 0);
    configureMatchup(session, FireAuthored, SpiritAuthored, FireAuthored,
                     FireAuthored, 100, 1); // one winning hit ends the fight

    test.assert(session.submitIntent({MenuIntentKind::SelectMove, 0}), true,
                "an authored attack submits");
    uint16_t revealTicks = 0;
    test.assert(runToResult(session, revealTicks), true, "the winning round reveals");
    test.assert(session.result().outcome, battle::Outcome::Win, "the foe is defeated");
    test.assert(session.state().feedback, static_cast<uint8_t>(duel::Feedback::Win),
                "a terminal win shows the win caption");
    const uint8_t bench1Hp = session.state().bench[0][0].hp;
    const uint8_t bench2Hp = session.state().bench[0][1].hp;

    uint16_t dwellTicks = 0;
    test.assert(runToBoundary(session, dwellTicks), true, "the win dwell settles");
    test.assert(session.exitReady(), true, "win is terminal");
    test.assert(player.creatureHPs[0], static_cast<uint8_t>(96),
                "active original slot syncs its post-round HP");
    test.assert(player.creatureHPs[1], bench1Hp, "living bench original slot 1 syncs");
    test.assert(player.creatureHPs[2], bench2Hp, "living bench original slot 2 syncs");
}

// A surviving wild round whose flee countdown hits zero is Fled; the footer
// shows the fled caption rather than a generic gather/round spelling.
void duelFleeCaption(Test &test) {
    preparePlayer();
    battle::BattleSession session;
    session.setRng({scriptedRng});
    rngDraws = 0;
    session.beginWild(4, 1, true, 0);
    battle::BattleState &state = session.stateForTest();
    state.gather.need = 8;
    state.gather.fleeTurns = 1;
    configureMatchup(session, FireAuthored, FireAuthored, FireAuthored,
                     FireAuthored, 100, 100);

    test.assert(session.submitIntent({MenuIntentKind::SelectMove, 0}), true,
                "a wild attack submits");
    uint16_t revealTicks = 0;
    test.assert(runToResult(session, revealTicks), true, "the round reveals");
    test.assert(session.result().outcome, battle::Outcome::Fled,
                "a spent flee countdown flees");
    test.assert(session.state().feedback, static_cast<uint8_t>(duel::Feedback::Fled),
                "the footer shows the fled caption");
    test.assert(session.state().gather.progress, static_cast<uint8_t>(0),
                "an attack round banks no gather progress");
}

// An empty or fully KO'd player party fails real setup. The duel has no Ready
// transition, so it must terminate at entry as a loss; the Flow then runs its
// normal defeat recovery (no opponent action, no win, no reward).
void duelExhaustedEntryIsLose(Test &test) {
    // Empty party: no occupied slot at all.
    player = Player();
    battle::BattleSession wildEmpty;
    wildEmpty.setRng({scriptedRng});
    wildEmpty.beginWild(4, 1, false, 0);
    test.assert(wildEmpty.isActive(), true, "empty-party wild entry stays active");
    test.assert(wildEmpty.awaitingPlayer(), false, "empty-party wild entry takes no input");
    test.assert(wildEmpty.exitReady(), true, "empty-party wild entry is terminal");
    test.assert(wildEmpty.result().outcome, battle::Outcome::Lose,
                "empty-party wild entry resolves as a loss");

    battle::BattleSession trainerEmpty;
    trainerEmpty.setRng({scriptedRng});
    trainerEmpty.beginTrainer(0);
    test.assert(trainerEmpty.exitReady(), true, "empty-party trainer entry is terminal");
    test.assert(trainerEmpty.result().outcome, battle::Outcome::Lose,
                "empty-party trainer entry resolves as a loss");

    // All-KO party: occupied slots but every stored HP is zero.
    preparePlayer();
    for (uint8_t slot = 0; slot < PARTY_SIZE; ++slot) player.creatureHPs[slot] = 0;
    battle::BattleSession wildKo;
    wildKo.setRng({scriptedRng});
    wildKo.beginWild(4, 1, false, 0);
    test.assert(wildKo.exitReady(), true, "all-KO wild entry is terminal");
    test.assert(wildKo.result().outcome, battle::Outcome::Lose,
                "all-KO wild entry resolves as a loss");

    battle::BattleSession trainerKo;
    trainerKo.setRng({scriptedRng});
    trainerKo.beginTrainer(0);
    test.assert(trainerKo.exitReady(), true, "all-KO trainer entry is terminal");
    test.assert(trainerKo.result().outcome, battle::Outcome::Lose,
                "all-KO trainer entry resolves as a loss");
}

// The bench attack-type mask is the member's authored field verbatim; an empty
// kit carries mask zero. A phantom spirit bit would invent a spirit counter
// (spirit beats fire) that the authored set never grants.
void duelBenchMaskSkipsEmptySlots(Test &test) {
    preparePlayer();
    // Two-member party: original slot 0 active, slot 1 the sole bench member.
    for (uint8_t slot = 2; slot < PARTY_SIZE; ++slot) player.party[slot].level = 0;
    player.party[0].level = 5;
    player.party[1].level = 5;
    player.creatureHPs[0] = 40;
    player.creatureHPs[1] = 40;

    const uint8_t plantBit =
        static_cast<uint8_t>(1u << duel::foldType(static_cast<uint8_t>(Type::PLANT)));
    player.party[1].attackMask = plantBit;

    battle::BattleSession session;
    session.setRng({scriptedRng});
    rngDraws = 0;
    session.beginWild(4, 1, false, 0);

    const uint8_t mask =
        session.state().moveUsesSpent[static_cast<uint8_t>(battle::Side::Player)][1];
    test.assert(mask, plantBit, "bench mask is exactly the real plant attack");
    test.assert(static_cast<uint8_t>(mask & 1u), static_cast<uint8_t>(0),
                "no phantom spirit bench bit");
    test.assert(static_cast<uint8_t>(mask & static_cast<uint8_t>(1u << 3)),
                static_cast<uint8_t>(0), "no phantom fire-counter bench bit");

    // Empty kit: the mask must be zero, and WAIT stays the only option.
    player.party[1].attackMask = 0;
    battle::BattleSession emptyKit;
    emptyKit.setRng({scriptedRng});
    emptyKit.beginWild(4, 1, false, 0);
    test.assert(emptyKit.state().moveUsesSpent[static_cast<uint8_t>(battle::Side::Player)][1],
                static_cast<uint8_t>(0), "empty bench mask is zero");
}

// A trade scales each side's OUTGOING hit by its own matchup: damage is what the
// side RECEIVES. Player Fire vs foe Water, both Spirit: player outgoing weak (3),
// foe outgoing bonus (9), so the player takes 9 and the foe takes 3.
void duelTradeScalesOutgoing(Test &test) {
    preparePlayer();
    battle::BattleSession session;
    session.setRng({scriptedRng});
    rngDraws = 0;
    session.beginWild(4, 1, false, 0);
    configureMatchup(session, FireAuthored, SpiritAuthored,
                     static_cast<uint8_t>(Type::WATER), SpiritAuthored, 100, 100);

    test.assert(session.submitIntent({MenuIntentKind::SelectMove, 0}), true,
                "the spirit mirror trade submits");
    uint16_t revealTicks = 0;
    test.assert(runToResult(session, revealTicks), true, "the trade reveals");
    test.assert(session.state().active[0].hp, static_cast<uint8_t>(91),
                "player receives the foe's bonus outgoing");
    test.assert(session.state().active[1].hp, static_cast<uint8_t>(97),
                "foe receives the player's weak outgoing");
    test.assert(session.state().damage[0], static_cast<uint8_t>(9),
                "footer damage[player] is the received bonus");
    test.assert(session.state().damage[1], static_cast<uint8_t>(3),
                "footer damage[foe] is the received weak");
    test.assert(session.state().feedback, static_cast<uint8_t>(duel::Feedback::Trade),
                "trade caption");
}

// --- jp8.11.7: gathered instance commit to the 64-slot collection ------------

// Empty store with a cold journal, then a real gatherable wild session ready for
// a one-round acquisition (need 1, tierRate 1).
void beginCommitSession() {
    flashFakeReset();
    preparePlayer();
    menu.clear();
    dialogMenu.clear();
    enterBattle();
    gameState.state = GameState_t::BATTLE;
    rngDraws = 0;
    battleSession().setRng({scriptedRng});
    battleSession().beginWild(4, 1, true, 1);
    battle::BattleState &state = battleSession().stateForTest();
    state.gather.need = 1;
    state.gather.progress = 0;
}

// Drive the live flow through a wild gather to the Gathered terminal.
void gatherToTerminal(uint16_t &frames) {
    BattleFlow::update(0);              // choice entry -> pick row
    BattleFlow::update(MENU_EDGE_B);    // open the wild action page (GATHER first)
    BattleFlow::update(MENU_EDGE_A);    // submit GATHER
    for (frames = 0; frames < 4096 && gameState.state != GameState_t::WORLD; ++frames)
        BattleFlow::update(0);
}

uint8_t occupiedSlots() {
    uint8_t count = 0;
    for (uint8_t i = 0; i < 64; ++i) {
        StoreRecord record;
        storeRead(i, record);
        if (record.id != 255) ++count;
    }
    return count;
}

// A successful gather commits exactly one duplicate-capable instance with the
// foe's real species/level/mask and its actual HP under the pinned byte7
// encoding; later world frames never re-commit.
void duelGatherCommitOnce(Test &test) {
    beginCommitSession();
    configureMatchup(battleSession(), SpiritAuthored, SpiritAuthored, FireAuthored,
                     SpiritAuthored, 100, 100);
    battle::BattleState &state = battleSession().stateForTest();
    battle::Combatant &foe = state.active[static_cast<uint8_t>(battle::Side::Opponent)];
    foe.maxHp = 100;
    foe.hp = 55; // a damaged foe must not be healed by the commit
    const uint8_t foeId = foe.id;
    const uint8_t foeLevel = foe.level;
    const uint8_t foeMask = foe.attackMask;

    uint16_t frames = 0;
    gatherToTerminal(frames);
    test.assert(gameState.state, GameState_t::WORLD,
                "the gather terminal returns to the world");

    StoreRecord committed = {};
    test.assert(storeRead(0, committed), true, "the committed instance is readable");
    test.assert(committed.id, foeId, "the gathered species is committed");
    test.assert(committed.exp, levelExpThreshold(foeLevel),
                "a fresh instance carries its level-threshold exp");
    test.assert(committed.attackMask, foeMask, "the folded attack mask is committed");
    test.assert(committed.hpEncoded, static_cast<uint8_t>(56),
                "a damaged foe uses the value-1 HP encoding");
    test.assert(storeInstanceHP(committed), static_cast<uint8_t>(55),
                "the gathered instance's actual HP is preserved");
    test.assert(occupiedSlots(), static_cast<uint8_t>(1),
                "exactly one instance is committed");

    for (uint8_t i = 0; i < 8; ++i) BattleFlow::update(0);
    test.assert(occupiedSlots(), static_cast<uint8_t>(1),
                "later world frames never re-commit");
}

// A full-HP foe commits the legacy/full zero byte, which reads back as the
// species' derived maximum for that level.
void duelGatherCommitFullHP(Test &test) {
    beginCommitSession();
    battle::BattleState &state = battleSession().stateForTest();
    const uint8_t realMax =
        state.active[static_cast<uint8_t>(battle::Side::Opponent)].maxHp;
    configureMatchup(battleSession(), SpiritAuthored, SpiritAuthored, FireAuthored,
                     SpiritAuthored, 100, realMax);
    battle::Combatant &foe = state.active[static_cast<uint8_t>(battle::Side::Opponent)];
    foe.hp = foe.maxHp = realMax;

    uint16_t frames = 0;
    gatherToTerminal(frames);
    StoreRecord committed = {};
    test.assert(storeRead(0, committed), true, "the full-HP instance is readable");
    test.assert(committed.hpEncoded, static_cast<uint8_t>(0),
                "a full foe encodes the legacy zero byte");
    test.assert(storeInstanceHP(committed), realMax,
                "a full instance reads back as its derived maximum");
}

// A committed instance survives an explicit v3 checkpoint and reboot: a fresh
// checkpoint folds the journal record into the store sector, and a boot re-reads
// the same species/exp/mask/HP.
void duelGatherCommitPersistence(Test &test) {
    beginCommitSession();
    configureMatchup(battleSession(), SpiritAuthored, SpiritAuthored, FireAuthored,
                     SpiritAuthored, 100, 100);
    battle::BattleState &state = battleSession().stateForTest();
    battle::Combatant &foe = state.active[static_cast<uint8_t>(battle::Side::Opponent)];
    foe.maxHp = 100;
    foe.hp = 42;
    const uint8_t foeId = foe.id;
    const uint8_t foeLevel = foe.level;
    const uint8_t foeMask = foe.attackMask;

    uint16_t frames = 0;
    gatherToTerminal(frames);

    SaveFile save = {};
    memset(save.partySlots, 255, 3);
    saveBegin();
    SaveStep step = SaveStep::Idle;
    for (uint16_t s = 0; s < 256 && saveInProgress(); ++s) step = saveStepAdvance(save);
    test.assert(step, SaveStep::Done, "the gather-commit checkpoint completes");

    SaveFile reboot = {};
    memset(reboot.partySlots, 255, 3);
    test.assert(storeBoot(reboot), true, "the gather-commit checkpoint reboots");
    StoreRecord restored = {};
    test.assert(storeRead(0, restored), true, "the committed instance reloads");
    test.assert(restored.id, foeId, "species survives reboot");
    test.assert(restored.exp, levelExpThreshold(foeLevel), "exp survives reboot");
    test.assert(restored.attackMask, foeMask, "mask survives reboot");
    test.assert(storeInstanceHP(restored), static_cast<uint8_t>(42), "HP survives reboot");
}

// A full 64-slot store refuses the gather commit cleanly: the collection is
// untouched and no aliasing or wrapping slot appears.
void duelGatherCommitFullStore(Test &test) {
    beginCommitSession();
    SaveFile save = {};
    memset(save.partySlots, 255, 3);
    StoreRecord filler = {};
    filler.id = 0;
    filler.exp = levelExpThreshold(3);
    filler.attackMask = 2;
    for (uint8_t i = 0; i < 64; ++i) {
        uint8_t slot = 255;
        test.assert(storeAdd(filler, slot), true, "fill the 64-slot collection");
        if (i % 16 == 15) {
            saveBegin();
            SaveStep step = SaveStep::Idle;
            for (uint16_t s = 0; s < 256 && saveInProgress(); ++s)
                step = saveStepAdvance(save);
            test.assert(step, SaveStep::Done, "the full collection checkpoint completes");
        }
    }
    test.assert(occupiedSlots(), static_cast<uint8_t>(64), "the collection is full");

    configureMatchup(battleSession(), SpiritAuthored, SpiritAuthored, FireAuthored,
                     SpiritAuthored, 100, 100);
    uint16_t frames = 0;
    gatherToTerminal(frames);

    test.assert(gameState.state, GameState_t::WORLD,
                "a full-store gather still ends cleanly");
    test.assert(occupiedSlots(), static_cast<uint8_t>(64),
                "a full store refuses without adding a slot");
    StoreRecord first = {};
    StoreRecord last = {};
    test.assert(storeRead(0, first), true, "the first record is readable");
    test.assert(storeRead(63, last), true, "the last record is readable");
    test.assert(first.id, static_cast<uint8_t>(0), "the first record is untouched");
    test.assert(last.id, static_cast<uint8_t>(0), "the last record is untouched");
}

// A gather that never reaches the need acquires nothing; escaping afterwards
// leaves the collection empty.
void duelGatherIncompleteNoCommit(Test &test) {
    beginCommitSession();
    battle::BattleState &state = battleSession().stateForTest();
    state.gather.need = 8;
    state.gather.progress = 0;
    configureMatchup(battleSession(), SpiritAuthored, SpiritAuthored, FireAuthored,
                     SpiritAuthored, 100, 100);

    BattleFlow::update(0);              // choice entry -> pick row
    BattleFlow::update(MENU_EDGE_B);    // wild action page
    BattleFlow::update(MENU_EDGE_A);    // submit GATHER
    for (uint16_t f = 0; f < 4096 && !battleSession().awaitingPlayer(); ++f)
        BattleFlow::update(0);
    test.assert(battleSession().state().gather.progress, static_cast<uint8_t>(1),
                "one gather banks partial progress without acquiring");

    BattleFlow::update(MENU_EDGE_B);    // wild action page
    BattleFlow::update(MENU_NAV_DOWN);  // cursor -> ESCAPE
    BattleFlow::update(MENU_EDGE_A);    // submit Escape
    for (uint16_t f = 0; f < 4096 && gameState.state != GameState_t::WORLD; ++f)
        BattleFlow::update(0);
    test.assert(gameState.state, GameState_t::WORLD, "the escape terminal returns to the world");
    test.assert(occupiedSlots(), static_cast<uint8_t>(0),
                "a gather that never acquires commits nothing");
}

// --- jp8.11.4: ordinary wild KO material award ------------------------------

// Install a two-array material fixture into the host FX fake: materialItem at
// `Rewards::materialItem` (0xFF = no authored material) and materialCount
// immediately after it, exactly as the packed Rewards mapping is laid out. The
// count offset is derived from the published addresses, never hardcoded.
void installMaterialFixture(uint8_t species, uint8_t itemId, uint8_t count) {
    const uint32_t countOffset =
        static_cast<uint32_t>(Rewards::materialCount - Rewards::materialItem);
    fxDataFake::dataBase = Rewards::materialItem;
    memset(fxDataFake::dataBytes, 0, sizeof(fxDataFake::dataBytes));
    fxDataFake::dataBytes[species] = itemId;
    fxDataFake::dataBytes[countOffset + species] = count;
}

// A live wild session ready for a one-hit Win: the player keeps its authored kit,
// the foe is a 1-HP Fire creature, and the player's Fire kit picks Spirit
// (spirit beats fire, 21 > 1) so the foe is KO'd on the reveal with no flee or
// gather path. `gatherable` selects the ordinary vs lure-zone encounter shape.
void beginWildKoSession(uint8_t foeSpecies, bool gatherable) {
    flashFakeReset();
    preparePlayer();
    player = Player();                       // fresh empty bag
    item::setBattleInventory(&player.inventory);
    menu.clear();
    dialogMenu.clear();
    enterBattle();
    gameState.state = GameState_t::BATTLE;
    rngDraws = 0;
    battleSession().setRng({scriptedRng});
    battleSession().beginWild(foeSpecies, 1, gatherable, gatherable ? 1 : 0);
    configureMatchup(battleSession(), FireAuthored, SpiritAuthored, FireAuthored,
                     FireAuthored, 100, 1);
}

// Drive the live flow through the Win dwell to the world terminal.
void wildKoToWorld() {
    BattleFlow::update(0);              // choice entry -> pick row
    BattleFlow::update(MENU_EDGE_A);    // commit the single attack cell
    for (uint16_t f = 0; f < 4096 && gameState.state != GameState_t::WORLD; ++f)
        BattleFlow::update(0);
}

// An ordinary wild KO awards exactly the authored material once; later world
// frames never re-award.
void duelWildKoAwardsMaterialOnce(Test &test) {
    installMaterialFixture(0, 8, 1); // species 0 -> one tier-2 material lure (id 8)
    beginWildKoSession(0, false);
    wildKoToWorld();

    test.assert(gameState.state, GameState_t::WORLD,
                "the wild KO terminal returns to the world");
    test.assert(item::inventoryCount(player.inventory, item::ItemKind::Lure, 8),
                static_cast<uint8_t>(1), "the authored material is awarded once");
    test.assert(item::inventoryNonZeroCount(player.inventory), static_cast<uint8_t>(1),
                "the award touches exactly one stack");

    for (uint8_t i = 0; i < 8; ++i) BattleFlow::update(0);
    test.assert(item::inventoryCount(player.inventory, item::ItemKind::Lure, 8),
                static_cast<uint8_t>(1), "later world frames never re-award");
}

// A 0xFF item sentinel (no authored material) awards nothing and leaves the bag
// empty: no phantom item.
void duelWildKoSentinelAwardsNothing(Test &test) {
    installMaterialFixture(4, 0xFF, 0); // no authored material for this species
    beginWildKoSession(4, false);
    wildKoToWorld();

    test.assert(gameState.state, GameState_t::WORLD, "the sentinel KO still exits");
    test.assert(item::inventoryNonZeroCount(player.inventory), static_cast<uint8_t>(0),
                "a sentinel species awards no phantom item");
}

// A gatherable-target KO is an explicit exclusion: the lure-zone encounter shape
// must award no creature material.
void duelGatherableKoAwardsNothing(Test &test) {
    installMaterialFixture(0, 8, 1);
    beginWildKoSession(0, true); // gatherable: a lure is deposited
    wildKoToWorld();

    test.assert(gameState.state, GameState_t::WORLD, "the gatherable KO still exits");
    test.assert(item::inventoryNonZeroCount(player.inventory), static_cast<uint8_t>(0),
                "a gatherable-target KO awards no creature material");
}

// A trainer/boss victory is an explicit exclusion even for the same authored
// species mapping.
void duelTrainerWinAwardsNothing(Test &test) {
    installMaterialFixture(0, 8, 1);
    beginWildKoSession(0, false);
    battleSession().stateForTest().trainer = true; // boss/trainer exclusion flag
    wildKoToWorld();

    test.assert(gameState.state, GameState_t::WORLD, "the trainer KO still exits");
    test.assert(item::inventoryNonZeroCount(player.inventory), static_cast<uint8_t>(0),
                "a trainer/boss victory awards nothing");
}

// A non-Win terminal awards nothing: a wild escape leaves the bag untouched.
void duelNonWinTerminalAwardsNothing(Test &test) {
    installMaterialFixture(0, 8, 1);
    beginWildKoSession(0, true); // gatherable wild offers Escape
    BattleFlow::update(0);              // choice entry -> pick row
    BattleFlow::update(MENU_EDGE_B);    // wild action page
    BattleFlow::update(MENU_NAV_DOWN);  // cursor -> ESCAPE
    BattleFlow::update(MENU_EDGE_A);    // submit Escape -> terminal
    for (uint16_t f = 0; f < 4096 && gameState.state != GameState_t::WORLD; ++f)
        BattleFlow::update(0);

    test.assert(gameState.state, GameState_t::WORLD, "the escape terminal returns to the world");
    test.assert(item::inventoryNonZeroCount(player.inventory), static_cast<uint8_t>(0),
                "a non-Win terminal awards nothing");
}

// At the 99-count cap the terminal still completes and the stack saturates: no
// wrap, no corruption and no extra stack.
void duelFullBagSaturatesOnAward(Test &test) {
    installMaterialFixture(0, 8, 1);
    beginWildKoSession(0, false);
    item::inventoryAdd(player.inventory, item::ItemKind::Lure, 8, 99); // fill to cap
    wildKoToWorld();

    test.assert(gameState.state, GameState_t::WORLD, "the full-bag KO still exits");
    test.assert(item::inventoryCount(player.inventory, item::ItemKind::Lure, 8),
                static_cast<uint8_t>(99), "a full bag stays at the cap");
    test.assert(item::inventoryNonZeroCount(player.inventory), static_cast<uint8_t>(1),
                "a full bag gains no phantom stack");
}

// --- jp8.11.10: ordinary wild-win experience and bounded level growth --------

// Install a real three-member beta party as stable store instances (slots 0..2)
// and mirror the same instances into the live Player, so an ordinary win has
// living occupied slots to grow. `exps` pins each member's stored cumulative
// experience; `missing` pins the HP missing from its derived maximum (0 = full,
// >= maximum = KO). Level/stats follow the stored exp through the existing model.
void seedPartyInstances(const uint16_t (&exps)[PARTY_SIZE],
                        const uint8_t (&missing)[PARTY_SIZE]) {
    flashFakeReset();
    battle::applyPlayerPreset(player, BattlePresets::beta);
    for (uint8_t i = 0; i < PARTY_SIZE; ++i) {
        player.party[i].level = levelFromExp(exps[i]);
        player.party[i].setStats(getCreatureFromStore(player.party[i].id));
        const uint8_t maxHp = player.party[i].statlist.hp;
        const uint8_t curHp = missing[i] >= maxHp ? 0 : static_cast<uint8_t>(maxHp - missing[i]);
        player.creatureHPs[i] = curHp;
        StoreRecord record = {};
        record.id = player.party[i].id;
        record.exp = exps[i];
        record.attackMask = static_cast<uint8_t>(player.party[i].attackMask & 0x1F);
        record.hpEncoded = curHp >= maxHp ? 0 : static_cast<uint8_t>(curHp + 1);
        storeWrite(i, record);
        storeAssign(i, i, curHp);
    }
}

// A wild session at a known foe level, ready for a one-hit Win: the foe is a
// 1-HP Fire creature and the player's Fire kit picks Spirit (spirit beats fire,
// 21 > 1) so the foe is KO'd on the reveal. `gatherable` selects the ordinary vs
// lure-zone encounter shape; the living hero starts at its derived maximum so a
// later checkpoint stays inside the model.
void beginWildWinSession(uint8_t foeLevel, bool gatherable) {
    menu.clear();
    dialogMenu.clear();
    enterBattle();
    gameState.state = GameState_t::BATTLE;
    rngDraws = 0;
    battleSession().setRng({scriptedRng});
    battleSession().beginWild(0, foeLevel, gatherable, gatherable ? 1 : 0);
    configureMatchup(battleSession(), FireAuthored, SpiritAuthored, FireAuthored,
                     FireAuthored, player.party[0].statlist.hp, 1);
}

uint16_t storedExp(uint8_t slot) {
    StoreRecord record;
    storeRead(slot, record);
    return record.exp;
}

const uint8_t kFull[PARTY_SIZE] = {0, 0, 0};

// An ordinary wild Win grants exactly 8 * opponentLevel to every living occupied
// member with no rounding drift; a member that stays below the next threshold
// keeps its level, its stored exp grows exactly, and its HP is untouched.
void duelWildWinAwardsExactExp(Test &test) {
    const uint16_t start = levelExpThreshold(3); // level 3
    const uint16_t exps[PARTY_SIZE] = {start, start, start};
    seedPartyInstances(exps, kFull);
    beginWildWinSession(4, false); // gain = 8 * 4 = 32
    wildKoToWorld();

    test.assert(gameState.state, GameState_t::WORLD, "the ordinary win returns to the world");
    const uint16_t expected = static_cast<uint16_t>(start + 32);
    test.assert(levelExpThreshold(4), static_cast<uint16_t>(64),
                "the next threshold is 64: the pinned total stays unrounded");
    for (uint8_t i = 0; i < PARTY_SIZE; ++i) {
        test.assert(storedExp(i), expected, "stored exp grows by exactly 8 * opponentLevel");
        test.assert(player.party[i].level, static_cast<uint8_t>(3),
                    "a member below the next threshold keeps its level");
    }
    test.assert(player.creatureHPs[1], player.party[1].statlist.hp,
                "a full member below the threshold keeps its current HP");
}

// The three threshold boundaries: one below, exactly at, and over the next
// threshold. A level increase installs the new derived level, and a member that
// levels up from full HP reaches its new derived maximum; a damaged member
// preserves its missing HP across the increase.
void duelWildWinLevelUpBoundaries(Test &test) {
    const uint16_t exps[PARTY_SIZE] = {
        31,   // +32 = 63 = threshold(4) - 1: no level up
        32,   // +32 = 64 = threshold(4): level 3 -> 4
        100,  // +32 = 132 > threshold(5): level 4 -> 5, remainder 7
    };
    const uint8_t missing[PARTY_SIZE] = {0, 0, 5};
    seedPartyInstances(exps, missing);
    beginWildWinSession(4, false); // gain = 32
    wildKoToWorld();

    test.assert(storedExp(0), static_cast<uint16_t>(63),
                "one below the threshold: the exact total is stored");
    test.assert(player.party[0].level, static_cast<uint8_t>(3),
                "one below the threshold: no level change");

    test.assert(storedExp(1), static_cast<uint16_t>(64),
                "exactly at the threshold: the exact total is stored");
    test.assert(player.party[1].level, static_cast<uint8_t>(4),
                "exactly at the threshold: the derived level increases");
    test.assert(player.creatureHPs[1], player.party[1].statlist.hp,
                "a full member that levels up reaches its new derived maximum");

    test.assert(storedExp(2), static_cast<uint16_t>(132),
                "over the threshold: the exact total keeps the remainder");
    test.assert(player.party[2].level, static_cast<uint8_t>(5),
                "over the threshold: the derived level increases");
    test.assert(player.creatureHPs[2],
                static_cast<uint8_t>(player.party[2].statlist.hp - 5),
                "a damaged member that levels up preserves its missing HP");
}

// Bounded growth: the stored exp saturates at the level-31 threshold and the
// derived level never exceeds 31.
void duelWildWinLevelCapAt31(Test &test) {
    const uint16_t cap = levelExpThreshold(LEVEL_COUNT - 1);
    const uint16_t exps[PARTY_SIZE] = {
        static_cast<uint16_t>(cap - 1), // level 30, +248 crosses the cap
        cap,                            // already at the cap
        levelExpThreshold(3),
    };
    seedPartyInstances(exps, kFull);
    beginWildWinSession(31, false); // gain = 8 * 31 = 248
    wildKoToWorld();

    test.assert(storedExp(0), cap, "exp saturates exactly at the level-31 threshold");
    test.assert(player.party[0].level, static_cast<uint8_t>(LEVEL_COUNT - 1),
                "the derived level caps at 31");
    test.assert(storedExp(1), cap, "an already-capped member stays at the threshold");
    test.assert(player.party[1].level, static_cast<uint8_t>(LEVEL_COUNT - 1),
                "an already-capped member stays at level 31");
    test.assert(storedExp(2), static_cast<uint16_t>(levelExpThreshold(3) + 248),
                "an uncapped member still grows by the exact amount");
}

// The award is a single terminal edge: later world frames never grow exp again.
void duelWildWinAwardsOnce(Test &test) {
    const uint16_t start = levelExpThreshold(3);
    const uint16_t exps[PARTY_SIZE] = {start, start, start};
    seedPartyInstances(exps, kFull);
    beginWildWinSession(4, false); // gain = 32
    wildKoToWorld();

    const uint16_t once = static_cast<uint16_t>(start + 32);
    test.assert(storedExp(0), once, "the first terminal awards once");
    for (uint8_t f = 0; f < 16; ++f) BattleFlow::update(0);
    for (uint8_t i = 0; i < PARTY_SIZE; ++i)
        test.assert(storedExp(i), once, "later world frames never re-award");
}

// Only an ordinary win grows exp: escape, gatherable-target and trainer/boss
// terminals all leave the stored experience exactly where it was.
void duelNonWinAwardsNoExp(Test &test) {
    const uint16_t start = levelExpThreshold(3);
    const uint16_t exps[PARTY_SIZE] = {start, start, start};

    // Wild Escape: a gatherable wild offers Escape on the action page.
    seedPartyInstances(exps, kFull);
    beginWildWinSession(4, true);
    BattleFlow::update(0);              // choice entry -> pick row
    BattleFlow::update(MENU_EDGE_B);    // wild action page
    BattleFlow::update(MENU_NAV_DOWN);  // cursor -> ESCAPE
    BattleFlow::update(MENU_EDGE_A);    // submit Escape -> terminal
    for (uint16_t f = 0; f < 4096 && gameState.state != GameState_t::WORLD; ++f)
        BattleFlow::update(0);
    test.assert(gameState.state, GameState_t::WORLD, "the escape terminal returns to the world");
    for (uint8_t i = 0; i < PARTY_SIZE; ++i)
        test.assert(storedExp(i), start, "an escape terminal awards no exp");

    // Gatherable-target KO still grants no creature exp.
    seedPartyInstances(exps, kFull);
    beginWildWinSession(4, true);
    wildKoToWorld();
    for (uint8_t i = 0; i < PARTY_SIZE; ++i)
        test.assert(storedExp(i), start, "a gatherable-target KO awards no exp");

    // Trainer/boss victory is not an ordinary wild win.
    seedPartyInstances(exps, kFull);
    beginWildWinSession(4, false);
    battleSession().stateForTest().trainer = true;
    wildKoToWorld();
    for (uint8_t i = 0; i < PARTY_SIZE; ++i)
        test.assert(storedExp(i), start, "a trainer/boss victory awards no exp");
}

// A KO'd party instance receives no reward and is never revived.
void duelWildWinKoMemberAwardsNothing(Test &test) {
    const uint16_t start = levelExpThreshold(3);
    const uint16_t exps[PARTY_SIZE] = {start, start, start};
    const uint8_t missing[PARTY_SIZE] = {0, 0, 255}; // member 2 is KO
    seedPartyInstances(exps, missing);
    beginWildWinSession(4, false);
    wildKoToWorld();

    test.assert(storedExp(0), static_cast<uint16_t>(start + 32), "a living member grows");
    test.assert(storedExp(2), start, "a KO member receives no exp");
    test.assert(player.creatureHPs[2], static_cast<uint8_t>(0), "a KO member is never revived");
}

// Accepted growth survives a v3 checkpoint and reboot exactly.
void duelWildWinExpSurvivesSave(Test &test) {
    const uint16_t exps[PARTY_SIZE] = {32, 32, 32}; // each levels 3 -> 4
    seedPartyInstances(exps, kFull);
    beginWildWinSession(4, false); // gain = 32 -> 64 = threshold(4)
    wildKoToWorld();

    SaveFile save = {};
    memset(save.partySlots, 255, 3);
    saveBegin();
    SaveStep step = SaveStep::Idle;
    for (uint16_t s = 0; s < 256 && saveInProgress(); ++s) step = saveStepAdvance(save);
    test.assert(step, SaveStep::Done, "the win checkpoint completes");

    SaveFile reboot = {};
    memset(reboot.partySlots, 255, 3);
    test.assert(storeBoot(reboot), true, "the win checkpoint reboots");
    for (uint8_t i = 0; i < PARTY_SIZE; ++i) {
        test.assert(storedExp(i), static_cast<uint16_t>(64),
                    "the accepted exact exp survives reboot");
    }
}

} // namespace duel_session_test_detail

inline void DuelSessionIntegrationSuite(TestRunner &runner) {    using namespace duel_session_test_detail;
    TestSuite suite("Duel session integration");

    Test commit(__func__);
    duelCommitBeforeSubmission(commit);
    suite.addTest(commit);

    Test flow(__func__);
    duelConcealAndDwellFlow(flow);
    suite.addTest(flow);

    Test entries(__func__);
    duelEntryFlagsConsumedSameFrame(entries);
    suite.addTest(entries);

    Test escapeEntry(__func__);
    duelEscapeEntryConsumedSameFrame(escapeEntry);
    suite.addTest(escapeEntry);

    Test round(__func__);
    duelAtomicAttackRound(round);
    suite.addTest(round);

    Test draw(__func__);
    duelSimultaneousDraw(draw);
    suite.addTest(draw);

    Test wait(__func__);
    duelWaitReceivesDefaultWin(wait);
    suite.addTest(wait);

    Test swapKo(__func__);
    duelSwapKoRevertsToOutgoing(swapKo);
    suite.addTest(swapKo);

    Test promote(__func__);
    duelOrdinaryKoPromotesFirstLiving(promote);
    suite.addTest(promote);

    Test gatherKo(__func__);
    duelGatherKoKeepsProgress(gatherKo);
    suite.addTest(gatherKo);

    Test gatherPause(__func__);
    duelGatherActiveKoPausesFleeCountdown(gatherPause);
    suite.addTest(gatherPause);

    Test gather(__func__);
    duelGatherAcquisition(gather);
    suite.addTest(gather);

    Test flee(__func__);
    duelFleeCaption(flee);
    suite.addTest(flee);

    Test escape(__func__);
    duelEscapeCancelsRound(escape);
    suite.addTest(escape);

    Test sync(__func__);
    duelOriginalSlotHpSync(sync);
    suite.addTest(sync);

    Test exhausted(__func__);
    duelExhaustedEntryIsLose(exhausted);
    suite.addTest(exhausted);

    Test benchMask(__func__);
    duelBenchMaskSkipsEmptySlots(benchMask);
    suite.addTest(benchMask);

    Test trade(__func__);
    duelTradeScalesOutgoing(trade);
    suite.addTest(trade);

    Test gatherCommit(__func__);
    duelGatherCommitOnce(gatherCommit);
    suite.addTest(gatherCommit);
    Test commitFull(__func__);
    duelGatherCommitFullHP(commitFull);
    suite.addTest(commitFull);

    Test persistence(__func__);
    duelGatherCommitPersistence(persistence);
    suite.addTest(persistence);

    Test commitFullStore(__func__);
    duelGatherCommitFullStore(commitFullStore);
    suite.addTest(commitFullStore);

    Test incomplete(__func__);
    duelGatherIncompleteNoCommit(incomplete);
    suite.addTest(incomplete);

    Test koAward(__func__);
    duelWildKoAwardsMaterialOnce(koAward);
    suite.addTest(koAward);

    Test koSentinel(__func__);
    duelWildKoSentinelAwardsNothing(koSentinel);
    suite.addTest(koSentinel);

    Test koGatherable(__func__);
    duelGatherableKoAwardsNothing(koGatherable);
    suite.addTest(koGatherable);

    Test koTrainer(__func__);
    duelTrainerWinAwardsNothing(koTrainer);
    suite.addTest(koTrainer);

    Test koNonWin(__func__);
    duelNonWinTerminalAwardsNothing(koNonWin);
    suite.addTest(koNonWin);

    Test koFullBag(__func__);
    duelFullBagSaturatesOnAward(koFullBag);
    suite.addTest(koFullBag);

    Test xpExact(__func__);
    duelWildWinAwardsExactExp(xpExact);
    suite.addTest(xpExact);

    Test xpBoundary(__func__);
    duelWildWinLevelUpBoundaries(xpBoundary);
    suite.addTest(xpBoundary);

    Test xpCap(__func__);
    duelWildWinLevelCapAt31(xpCap);
    suite.addTest(xpCap);

    Test xpOnce(__func__);
    duelWildWinAwardsOnce(xpOnce);
    suite.addTest(xpOnce);

    Test xpNonWin(__func__);
    duelNonWinAwardsNoExp(xpNonWin);
    suite.addTest(xpNonWin);

    Test xpKo(__func__);
    duelWildWinKoMemberAwardsNothing(xpKo);
    suite.addTest(xpKo);

    Test xpSave(__func__);
    duelWildWinExpSurvivesSave(xpSave);
    suite.addTest(xpSave);

    runner.addTestSuite(suite);
}

#endif // CGFX_DUEL_BATTLE
