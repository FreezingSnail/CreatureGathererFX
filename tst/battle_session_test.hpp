#pragma once

#include "test.hpp"
#include "../src/engine/battle/BattleSession.hpp"
#include "../src/player/Player.hpp"

extern Player player;

inline void BattleSessionIntegrationTest(TestSuite &suite)
{
    using namespace battle;
    Test test(__func__);
    static_assert(sizeof(TurnCursor) == 9, "cursor ABI remains nine bytes");
#ifdef __AVR__
    static_assert(sizeof(BattleSession) == 132, "session ABI remains compact");
#endif

    player = Player();
    player.loadCreature(0, 1);
    player.loadCreature(1, 2);
    player.loadCreature(2, 3);
    player.creatureHPs[0] = 73;
    player.creatureHPs[1] = 41;
    player.creatureHPs[2] = 29;

    BattleSession session;
    test.assert(session.isActive(), false, "constructed session is inactive");
    test.assert(session.awaitingPlayer(), false, "constructed session accepts no intent");

    session.beginWild(4, 1, true, 1);
    test.assert(session.isActive(), true, "wild session is active");
    test.assert(session.awaitingPlayer(), true, "wild session begins at choice");
    test.assert(session.awaitingReplacement(), false, "wild entry does not await replacement");
    test.assert(player.creatureHPs[0], static_cast<uint8_t>(73),
                "wild entry preserves persistent HP");

    const BattleView opened = session.view();
    test.assert(opened.partyCount[0], static_cast<uint8_t>(3),
                "view publishes original player party count");
    test.assert(opened.activeSlot[0], static_cast<uint8_t>(0),
                "view publishes active original slot");
    test.assert(opened.party[0][1].id, player.party[1].id,
                "view keeps bench identity by original slot");
    test.assert(opened.party[0][1].hp, static_cast<uint8_t>(41),
                "view keeps bench HP by original slot");
    test.assert(session.partyChoices().choices[0].slot, static_cast<uint8_t>(1),
                "party snapshot carries original slot, not cursor");

    test.assert(session.submitIntent({MenuIntentKind::Escape, 0}), true,
                "choice accepts wild escape");
    test.assert(session.advance(), true, "advance emits one escape result");
    test.assert(session.result().kind, ResultKind::Escape,
                "escape result kind is stable");
    test.assert(session.result().outcome, Outcome::Escaped,
                "wild escape is terminal");
    test.assert(session.advance(), false,
                "playback blocks a second result before acknowledgement");
    session.finishPresentation();
    const Outcome escaped = session.result().outcome;
    session.finishPresentation();
    test.assert(session.result().outcome, escaped,
                "presentation acknowledgement is exactly once");
    test.assert(session.exitReady(), true, "terminal feedback is acknowledged");
    test.assert(session.isActive(), true,
                "session stays resident until mode exit");
    test.assert(session.submitIntent({MenuIntentKind::Gather, 0}), false,
                "terminal session ignores new intents");

    // Native headless lifecycle: a refused gather still consumes one player
    // action, freezes the opponent choice, and reaches one end-turn result.
    session.beginWild(4, 1, false, 0);
    session.setRng({nullptr});
    test.assert(session.submitIntent({MenuIntentKind::Back, 0}), false,
                "Back never submits a battle action");
    test.assert(session.submitIntent({MenuIntentKind::None, 0}), false,
                "None never submits a battle action");
    test.assert(session.submitIntent({MenuIntentKind::Gather, 0}), true,
                "gather intent submits as one action even when refused");
    uint8_t results = 0;
    for (uint8_t step = 0; step < 8 && session.isActive() &&
         !session.awaitingPlayer(); ++step) {
        if (!session.advance()) break;
        ++results;
        session.finishPresentation();
    }
    test.assert(results != 0, true, "native session emits a result sequence");
    test.assert(session.awaitingPlayer() || session.exitReady(), true,
                "one-action sequence returns to choice or terminal");

    // A faint caused by the end-turn tick requires replacement before a new
    // choice. Once the replacement result is acknowledged, the cursor opens a
    // fresh choice instead of remaining at the completed end-turn cursor.
    session.beginWild(4, 1, true, 1);
    battle::BattleState &fixture = session.stateForTest();
    fixture.active[0].hp = 1;
    fixture.active[0].status.effects[0] = Effect::SAPPD;
    for (uint8_t side = 0; side < 2; ++side) {
        for (uint8_t slot = 0; slot < 4; ++slot) {
            fixture.active[side].moveIds[slot] = 255;
        }
    }
    test.assert(session.submitIntent({MenuIntentKind::SelectMove, 0}), true,
                "tick replacement fixture accepts a choice");
    for (uint8_t step = 0; step < 5 && !session.awaitingPlayer() &&
         !session.exitReady(); ++step) {
        test.assert(session.advance(), true, "fixture emits one acknowledged result");
        session.finishPresentation();
    }
    test.assert(session.awaitingReplacement(), true,
                "end-turn faint waits for replacement");
    test.assert(session.submitIntent({MenuIntentKind::SelectParty, 1}), true,
                "replacement accepts original party slot");
    test.assert(session.advance(), true, "replacement emits a switch result");
    test.assert(session.result().kind, ResultKind::Switch,
                "replacement result remains separate from end turn");
    session.finishPresentation();
    test.assert(session.awaitingPlayer(), true,
                "replacement completion opens a fresh choice");

    suite.addTest(test);
}

inline void BattleSessionSuite(TestRunner &runner)
{
    TestSuite suite("Battle session integration");
    BattleSessionIntegrationTest(suite);
    runner.addTestSuite(suite);
}
