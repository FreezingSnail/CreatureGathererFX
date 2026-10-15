#pragma once

#include "test.hpp"
#include "../src/engine/battle/BattleMenu.hpp"
#include "../src/engine/battle/BattleSession.hpp"
#include "../src/engine/menu/MenuNav.hpp"
#include "../src/player/Player.hpp"

extern Player player;

namespace battle_slim_session_test_detail {

using namespace battle;

inline void prepareKnockout(BattleSession &session)
{
    player = Player();
    player.loadCreature(0, 1);
    player.loadCreature(1, 2);
    player.loadCreature(2, 3);
    session.beginWild(4, 1, false, 0);
    session.setRng({nullptr});

    battle::BattleState &state = session.stateForTest();
    Combatant &playerActive = state.active[static_cast<uint8_t>(Side::Player)];
    Combatant &opponent = state.active[static_cast<uint8_t>(Side::Opponent)];
    playerActive.hp = 1;
    playerActive.stats.speed = 1;
    playerActive.moveIds[0] = 0;
    playerActive.moves[0] = Move(MoveBitSet{
        static_cast<uint8_t>(Type::SPIRIT), 1, 1, 0, 0
    });

    opponent.stats.speed = 255;
    opponent.moveIds[0] = 0;
    opponent.moves[0] = Move(MoveBitSet{
        static_cast<uint8_t>(Type::SPIRIT), 255, 1, 0, 0
    });

    // Original slot one is fainted; slot two is the only replacement. Its
    // visible choice row is zero, so this checks that ownership stays original.
    state.bench[static_cast<uint8_t>(Side::Player)][0].hp = 0;
    state.bench[static_cast<uint8_t>(Side::Player)][1].hp = 50;
    StatModifer incomingModifiers;
    incomingModifiers.setModifier(StatType::ATTACK_M, 1);
    state.partyModifiers[static_cast<uint8_t>(Side::Player)][2] =
        incomingModifiers.modifiers;
    state.moveUsesSpent[static_cast<uint8_t>(Side::Player)][2] = 1;
}

inline void VoluntarySwitchIsAccepted(TestSuite &suite)
{
    using namespace battle;
    Test test(__func__);
    BattleSession session;
    prepareKnockout(session);

    battle::BattleState &state = session.stateForTest();
    state.active[static_cast<uint8_t>(Side::Player)].hp = 40;
    test.assert(session.submitIntent({MenuIntentKind::SelectParty, 2}), true,
                "ordinary action choice accepts a living original bench slot");
    test.assert(session.submitIntent({MenuIntentKind::SelectParty, 1}), false,
                "a fainted original bench slot is rejected");
    test.assert(session.advance(), true,
                "switch emits one transition result");
    test.assert(static_cast<uint8_t>(session.result().kind),
                static_cast<uint8_t>(ResultKind::Switch),
                "switch result owns the presentation phase");
    test.assert(state.activeSlot[static_cast<uint8_t>(Side::Player)],
                static_cast<uint8_t>(2),
                "incoming creature occupies its original bench slot");
    session.finishPresentation();
    suite.addTest(test);
}

inline void KnockoutReplacementPolicy(TestSuite &suite)
{
    using namespace battle;
    Test test(__func__);
    BattleSession session;
    prepareKnockout(session);

    test.assert(session.submitIntent({MenuIntentKind::SelectMove, 0}), true,
                "knockout fixture submits its frozen player action");
    test.assert(session.advance(), true,
                "faster opponent resolves before the scheduled player action");
    test.assert(session.isPresenting(), true,
                "session exposes presentation phase without extra state");
    test.assert((session.result().flags & PLAYER_FAINTED) != 0, true,
                "opponent action knocks out the active player creature");
    session.finishPresentation();
    const ReplacementChoices sole = session.replacementChoices();
    test.assert(sole.count, static_cast<uint8_t>(1),
                "compact handoff excludes the fainted first bench member");
    test.assert(sole.slots[0], static_cast<uint8_t>(2),
                "compact handoff keeps the second survivor's original slot");
    test.assert(session.isPresenting(), false,
                "presentation acknowledgement leaves the presentation phase");
    test.assert(session.state().over, false,
                "surviving bench prevents terminal defeat");

#if CGFX_BATTLE_AUTO_REPLACE
    test.assert(session.awaitingReplacement(), false,
                "automatic policy does not wait for player replacement");
    test.assert(session.submitIntent({MenuIntentKind::SelectParty, 2}), false,
                "automatic policy ignores direct party selection");
    test.assert(session.advance(), true,
                "automatic policy enters the forced replacement result");
    test.assert(session.result().kind, ResultKind::Switch,
                "forced replacement remains an explicit switch result");
    test.assert(session.state().activeSlot[static_cast<uint8_t>(Side::Player)],
                static_cast<uint8_t>(2),
                "automatic policy chooses lowest living original slot");
    test.assert(session.state().moveUsesSpent[static_cast<uint8_t>(Side::Player)][2],
                static_cast<uint8_t>(1),
                "replacement keeps move uses owned by its original slot");
    test.assert(session.state().active[static_cast<uint8_t>(Side::Player)]
                    .statMods.getModifier(StatType::ATTACK_M), 1,
                "replacement restores modifiers from its original slot");
#else
    test.assert(session.advance(), false,
                "choice policy waits for direct replacement selection");
    test.assert(session.submitIntent({MenuIntentKind::SelectParty, 1}), false,
                "choice policy rejects a fainted bench member");
    BattleMenu menu;
    menu.beginReplacement(sole);
    const MenuIntent selected = menu.update((1u << 3));
    test.assert(selected.kind, MenuIntentKind::SelectParty,
                "filtered replacement menu emits a party selection");
    test.assert(selected.index, static_cast<uint8_t>(2),
                "sole replacement menu keeps its original slot");
    test.assert(session.submitIntent(selected), true,
                "session accepts the paused menu's valid replacement intent");
    test.assert(session.advance(), true,
                "choice policy enters the forced replacement result");
    test.assert(session.result().kind, ResultKind::Switch,
                "forced replacement remains an explicit switch result");
    test.assert(session.state().activeSlot[static_cast<uint8_t>(Side::Player)],
                static_cast<uint8_t>(2),
                "visible row choice resolves to original party slot two");
    test.assert(session.state().moveUsesSpent[static_cast<uint8_t>(Side::Player)][2],
                static_cast<uint8_t>(1),
                "replacement keeps move uses owned by its original slot");
    test.assert(session.state().active[static_cast<uint8_t>(Side::Player)]
                    .statMods.getModifier(StatType::ATTACK_M), 1,
                "replacement restores modifiers from its original slot");
#endif

    session.finishPresentation();
    test.assert(session.advance(), true,
                "frozen turn cursor advances after replacement presentation");
    test.assert(session.result().kind, ResultKind::EndTurn,
                "knocked-out scheduled actor does not receive an extra action");
    suite.addTest(test);
}

inline void TwoBenchReplacementUsesPolicy(TestSuite &suite)
{
    using namespace battle;
    Test test(__func__);
    BattleSession session;
    prepareKnockout(session);
    session.stateForTest().bench[static_cast<uint8_t>(Side::Player)][0].hp = 40;

    test.assert(session.submitIntent({MenuIntentKind::SelectMove, 0}), true,
                "two-bench fixture submits its frozen action");
    test.assert(session.advance(), true,
                "opponent knocks out the scheduled player actor");
    session.finishPresentation();
    const ReplacementChoices compact = session.replacementChoices();
    test.assert(compact.count, static_cast<uint8_t>(2),
                "compact menu handoff contains both living bench rows");
    test.assert(compact.slots[0], static_cast<uint8_t>(1),
                "compact first row retains original slot one");
    test.assert(compact.slots[1], static_cast<uint8_t>(2),
                "compact second row retains original slot two");
    test.assert(compact.ids[1], session.state().bench[0][1].id,
                "compact row carries the selected creature ID");
#if CGFX_BATTLE_AUTO_REPLACE
    test.assert(session.advance(), true,
                "automatic mode replaces without a menu");
    test.assert(session.state().activeSlot[0], static_cast<uint8_t>(1),
                "automatic mode picks the lowest living original slot");
#else
    test.assert(session.awaitingReplacement(), true,
                "choice mode presents both living bench members");
    const PartySnapshot choices = session.partyChoices();
    test.assert(choices.count, static_cast<uint8_t>(2),
                "choice mode includes both survivors");
    test.assert(choices.choices[0].slot, static_cast<uint8_t>(1),
                "first row carries original slot one");
    test.assert(choices.choices[1].slot, static_cast<uint8_t>(2),
                "second row carries original slot two");
    BattleMenu menu;
    menu.beginReplacement(compact);
    menu.update(MENU_NAV_DOWN);
    const MenuIntent selected = menu.update((1u << 3));
    test.assert(selected.kind, MenuIntentKind::SelectParty,
                "filtered menu emits the selected second row");
    test.assert(selected.index, static_cast<uint8_t>(2),
                "second visible row retains its original slot");
    test.assert(session.submitIntent(selected), true,
                "session accepts the valid paused menu handoff");
    test.assert(session.advance(), true,
                "choice mode emits the selected forced replacement");
    test.assert(session.state().activeSlot[0], static_cast<uint8_t>(2),
                "selected original slot is not confused with row index");
#endif
    test.assert(session.result().kind, ResultKind::Switch,
                "both policies retain a separate replacement result");
    suite.addTest(test);
}

inline void ReplacementRowsMapAroundMiddleActiveSlot(TestSuite &suite)
{
    using namespace battle;
    BattleSession session;
    session.beginWild(4, 1, false, 0);
    battle::BattleState &state = session.stateForTest();
    state.partyCount[static_cast<uint8_t>(Side::Player)] = 3;
    state.activeSlot[static_cast<uint8_t>(Side::Player)] = 1;
    state.bench[static_cast<uint8_t>(Side::Player)][0] = {6, 1, 20};
    state.bench[static_cast<uint8_t>(Side::Player)][1] = {7, 1, 30};

    const ReplacementChoices rows = session.replacementChoices();
    Test test(__func__);
    test.assert(rows.count, static_cast<uint8_t>(2),
                "middle-active snapshot contains both bench rows");
    test.assert(rows.slots[0], static_cast<uint8_t>(0),
                "bench before active maps to original slot zero");
    test.assert(rows.slots[1], static_cast<uint8_t>(2),
                "bench after active maps to original slot two");
    test.assert(rows.ids[0], static_cast<uint8_t>(6),
                "first compact row retains its creature ID");
    test.assert(rows.ids[1], static_cast<uint8_t>(7),
                "second compact row retains its creature ID");
    suite.addTest(test);
}

inline void ExhaustedPartyEndsBattle(TestSuite &suite)
{
    using namespace battle;
    Test test(__func__);
    BattleSession session;
    prepareKnockout(session);
    battle::BattleState &state = session.stateForTest();
    state.bench[static_cast<uint8_t>(Side::Player)][0].hp = 0;
    state.bench[static_cast<uint8_t>(Side::Player)][1].hp = 0;
    test.assert(session.replacementChoices().count, static_cast<uint8_t>(0),
                "compact handoff is empty when the party has no survivors");

    test.assert(session.submitIntent({MenuIntentKind::SelectMove, 0}), true,
                "exhaustion fixture submits its frozen action");
    test.assert(session.advance(), true,
                "opponent action knocks out the last living party member");
    test.assert(session.result().outcome, Outcome::Lose,
                "party exhaustion is terminal");
    session.finishPresentation();
    test.assert(session.exitReady(), true,
                "terminal acknowledgement reaches exit exactly once");
    test.assert(player.creatureHPs[0], static_cast<uint8_t>(0),
                "terminal acknowledgement synchronizes active HP");
    session.finishPresentation();
    test.assert(session.exitReady(), true,
                "repeated acknowledgement does not advance terminal state");
    suite.addTest(test);
}

inline void OpponentReplacementIsForced(TestSuite &suite)
{
    using namespace battle;
    Test test(__func__);
    player = Player();
    player.loadCreature(0, 1);
    player.loadCreature(1, 2);
    BattleSession session;
    session.beginTrainer(0);
    session.setRng({nullptr});
    session.stateForTest().active[static_cast<uint8_t>(Side::Opponent)].hp = 0;

    test.assert(session.submitIntent({MenuIntentKind::SelectMove, 0}), true,
                "live player submits while opponent awaits replacement");
    test.assert(session.advance(), true,
                "opponent replacement precedes the frozen turn actions");
    test.assert(session.result().kind, ResultKind::Switch,
                "opponent replacement emits the switch result");
    test.assert(session.result().actor, Side::Opponent,
                "forced replacement records the opponent as actor");
    test.assert((session.result().flags & FORCED_SWITCH) != 0, true,
                "opponent switch uses the forced feedback path");
    test.assert(session.state().activeSlot[static_cast<uint8_t>(Side::Opponent)],
                static_cast<uint8_t>(1),
                "opponent chooses the lowest living original slot");
    suite.addTest(test);
}

inline void BattleSlimSessionSuite(TestRunner &runner)
{
    TestSuite suite("Slim battle session and replacement policy");
    VoluntarySwitchIsAccepted(suite);
    KnockoutReplacementPolicy(suite);
    TwoBenchReplacementUsesPolicy(suite);
    ReplacementRowsMapAroundMiddleActiveSlot(suite);
    ExhaustedPartyEndsBattle(suite);
    OpponentReplacementIsForced(suite);
    runner.addTestSuite(suite);
}

} // namespace battle_slim_session_test_detail
