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

#include "test.hpp"
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
    hero.moveTypes[0] = playerMove;

    battle::Combatant &foe = state.active[static_cast<uint8_t>(battle::Side::Opponent)];
    foe.types = DualType(static_cast<Type>(opponentType));
    foe.hp = foe.maxHp = opponentHp;
    for (uint8_t slot = 0; slot < 4; ++slot)
        if (foe.moveIds[slot] != EMPTY_MOVE_ID) foe.moveTypes[slot] = opponentMove;
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
    for (uint8_t slot = 0; slot < 4; ++slot) {
        state.active[0].moveTypes[slot] = EMPTY_MOVE_ID;
        state.active[0].moveIds[slot] = EMPTY_MOVE_ID;
    }
    state.active[0].types = DualType(static_cast<Type>(FireAuthored));
    state.active[0].hp = state.active[0].maxHp = 100;
    state.active[1].types = DualType(static_cast<Type>(FireAuthored));
    state.active[1].hp = state.active[1].maxHp = 100;
    for (uint8_t slot = 0; slot < 4; ++slot)
        if (state.active[1].moveIds[slot] != EMPTY_MOVE_ID)
            state.active[1].moveTypes[slot] = SpiritAuthored;

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
    for (uint8_t slot = 0; slot < 4; ++slot)
        if (state.active[1].moveIds[slot] != EMPTY_MOVE_ID)
            state.active[1].moveTypes[slot] = SpiritAuthored;

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

// The bench attack-type mask must derive from the real kit IDs. An empty slot
// leaves moveList defaulted to a Move() that reads as authored SPIRIT, so a mask
// that ignores the empty sentinel would invent a phantom spirit counter (and
// spirit beats fire, a phantom counter to a fire pick).
void duelBenchMaskSkipsEmptySlots(Test &test) {
    preparePlayer();
    // Two-member party: original slot 0 active, slot 1 the sole bench member.
    for (uint8_t slot = 2; slot < PARTY_SIZE; ++slot) player.party[slot].level = 0;
    player.party[0].level = 5;
    player.party[1].level = 5;
    player.creatureHPs[0] = 40;
    player.creatureHPs[1] = 40;

    // Bench member: one real plant attack, every other slot an empty sentinel.
    player.party[1].moves[0] = 1; // a valid authored move id
    player.party[1].moveList[0] =
        Move(MoveBitSet{static_cast<uint8_t>(Type::PLANT), 10, 0, 0, 0});
    for (uint8_t slot = 1; slot < 4; ++slot) {
        player.party[1].moves[slot] = EMPTY_MOVE_ID;
        player.party[1].moveList[slot] = Move();
    }

    battle::BattleSession session;
    session.setRng({scriptedRng});
    rngDraws = 0;
    session.beginWild(4, 1, false, 0);

    const uint8_t plantBit =
        static_cast<uint8_t>(1u << duel::foldType(static_cast<uint8_t>(Type::PLANT)));
    const uint8_t mask =
        session.state().moveUsesSpent[static_cast<uint8_t>(battle::Side::Player)][1];
    test.assert(mask, plantBit, "bench mask is exactly the real plant attack");
    test.assert(static_cast<uint8_t>(mask & 1u), static_cast<uint8_t>(0),
                "no phantom spirit bench bit from the empty slots");
    test.assert(static_cast<uint8_t>(mask & static_cast<uint8_t>(1u << 3)),
                static_cast<uint8_t>(0), "no phantom fire-counter bench bit");

    // All-empty kit: the mask must be zero, and WAIT stays the only option.
    for (uint8_t slot = 0; slot < 4; ++slot) {
        player.party[1].moves[slot] = EMPTY_MOVE_ID;
        player.party[1].moveList[slot] = Move();
    }
    player.creatureHPs[0] = 40;
    player.creatureHPs[1] = 40;
    battle::BattleSession emptyKit;
    emptyKit.setRng({scriptedRng});
    emptyKit.beginWild(4, 1, false, 0);
    test.assert(emptyKit.state().moveUsesSpent[static_cast<uint8_t>(battle::Side::Player)][1],
                static_cast<uint8_t>(0), "all-empty bench mask is zero");
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

    runner.addTestSuite(suite);
}

#endif // CGFX_DUEL_BATTLE
