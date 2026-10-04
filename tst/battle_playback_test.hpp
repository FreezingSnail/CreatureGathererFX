#pragma once

#include "test.hpp"
#include "../src/engine/battle/BattlePresenter.hpp"
#include "../src/engine/battle/BattleSession.hpp"
#include "../src/lib/FxReadCounter.hpp"
#include "../src/player/Player.hpp"

extern Player player;

namespace battle_playback_test_detail {

uint8_t rngValues[16] = {};
uint8_t rngLength = 0;
uint8_t rngIndex = 0;
uint8_t rngCalls = 0;

void resetRng(uint8_t first = 0, uint8_t second = 0,
             uint8_t third = 0, uint8_t fourth = 0)
{
    rngValues[0] = first;
    rngValues[1] = second;
    rngValues[2] = third;
    rngValues[3] = fourth;
    rngLength = 4;
    rngIndex = 0;
    rngCalls = 0;
}

uint8_t scriptedRng(uint8_t)
{
    ++rngCalls;
    return rngIndex < rngLength ? rngValues[rngIndex++] : 0;
}

void preparePlayer()
{
    player = Player();
    player.loadCreature(0, 1);
    player.loadCreature(1, 2);
    player.loadCreature(2, 3);
    player.creatureHPs[0] = 73;
    player.creatureHPs[1] = 41;
    player.creatureHPs[2] = 29;
}

struct Driver {
    battle::BattleSession session;
    battle::BattlePresenter presenter;
    bool resident = false;
    uint8_t resultKinds[16] = {};
    uint8_t resultCount = 0;

    bool frame(bool freshAEdge)
    {
        if (resident) {
            if (presenter.done()) {
                session.finishPresentation();
                presenter.reset();
                resident = false;
            } else {
                presenter.update(freshAEdge);
            }
            return true;
        }
        if (!session.isActive() || session.exitReady() ||
            session.awaitingPlayer()) {
            return false;
        }
        if (!session.advance()) return false;
        if (resultCount < sizeof(resultKinds)) {
            resultKinds[resultCount++] =
                static_cast<uint8_t>(session.result().kind);
        }
        presenter.begin(session.result());
        FxReadCounter::resetFrame();
        resident = true;
        return true;
    }

    bool submit(MenuIntent intent)
    {
        return session.submitIntent(intent);
    }

    uint16_t drain(bool accelerated)
    {
        uint16_t frames = 0;
        while (frames < 4096 && session.isActive() &&
               !session.awaitingPlayer() && !session.exitReady()) {
            frame(accelerated);
            ++frames;
        }
        return frames;
    }
};

void assertSameCombatState(Test &test, const battle::BattleState &a,
                           const battle::BattleState &b)
{
    for (uint8_t side = 0; side < 2; ++side) {
        test.assert(a.active[side].hp, b.active[side].hp,
                    "natural and accelerated active HP match");
        test.assert(a.active[side].id, b.active[side].id,
                    "natural and accelerated active identity matches");
        test.assert(a.activeSlot[side], b.activeSlot[side],
                    "natural and accelerated active slot matches");
    }
    test.assert(a.gather.progress, b.gather.progress,
                "natural and accelerated gather progress matches");
    test.assert(a.gather.fleeTurns, b.gather.fleeTurns,
                "natural and accelerated flee countdown matches");
    test.assert(a.over, b.over, "natural and accelerated terminal state matches");
}

} // namespace battle_playback_test_detail

inline void BattlePlaybackIntegrationTest(TestSuite &suite)
{
    using namespace battle;
    using namespace battle_playback_test_detail;
    Test test(__func__);

    preparePlayer();
    resetRng();
    Driver natural;
    natural.session.setRng({scriptedRng});
    natural.session.beginWild(4, 1, true, 1);
    test.assert(natural.submit({MenuIntentKind::SelectMove, 0}), true,
                "wild choice submits exactly one move intent");
    test.assert(natural.frame(false), true,
                "choice completion starts one resident playback result");
    const uint8_t callsAfterResolution = rngCalls;
    natural.frame(false);
    test.assert(rngCalls, callsAfterResolution,
                "repeated playback frames do not reroll resolution");
    natural.drain(false);
    const battle::BattleState naturalState = natural.session.state();
    const uint8_t naturalCalls = rngCalls;
    const uint8_t naturalResultCount = natural.resultCount;

    preparePlayer();
    resetRng();
    Driver accelerated;
    accelerated.session.setRng({scriptedRng});
    accelerated.session.beginWild(4, 1, true, 1);
    FxReadCounter::resetFrame();
    test.assert(accelerated.submit({MenuIntentKind::SelectMove, 0}), true,
                "accelerated wild choice submits the same move");
    accelerated.frame(false);
    FxReadCounter::resetFrame();
    accelerated.drain(true);
    const battle::BattleState acceleratedState = accelerated.session.state();
    test.assert(rngCalls, naturalCalls,
                "accelerated and natural playback use identical RNG count");
    assertSameCombatState(test, naturalState, acceleratedState);
    test.assert(accelerated.resultCount, naturalResultCount,
                "accelerated and natural playback keep result count");
    for (uint8_t i = 0; i < naturalResultCount && i < 16; ++i) {
        test.assert(accelerated.resultKinds[i], natural.resultKinds[i],
                    "accelerated and natural playback keep result order");
    }
    test.assert(FxReadCounter::count(), static_cast<uint8_t>(0),
                "steady native playback performs zero FX reads");

    // Trainer setup publishes all three opponent slots; one action still
    // advances through playback before the next choice can open.
    preparePlayer();
    resetRng();
    Driver trainer;
    trainer.session.setRng({scriptedRng});
    trainer.session.beginTrainer(0);
    test.assert(trainer.session.state().partyCount[1], static_cast<uint8_t>(3),
                "trainer playback fixture is a three-creature battle");
    test.assert(trainer.submit({MenuIntentKind::SelectMove, 0}), true,
                "trainer move choice submits");
    trainer.frame(false);
    trainer.drain(true);
    test.assert(trainer.session.awaitingPlayer() || trainer.session.exitReady(), true,
                "trainer playback returns only after action/end-turn completion");

    // End-turn faint opens replacement only after its resident result has been
    // acknowledged. Back, dead, and out-of-range choices never escape it.
    preparePlayer();
    Driver replacement;
    replacement.session.beginWild(4, 1, true, 1);
    battle::BattleState &fixture = replacement.session.stateForTest();
    fixture.active[0].hp = 1;
    fixture.active[0].status.effects[0] = Effect::SAPPD;
    for (uint8_t side = 0; side < 2; ++side) {
        for (uint8_t slot = 0; slot < 4; ++slot) {
            fixture.active[side].moveIds[slot] = 255;
        }
    }
    test.assert(replacement.submit({MenuIntentKind::SelectMove, 0}), true,
                "replacement fixture accepts initial action");
    replacement.frame(false);
    replacement.drain(true);
    test.assert(replacement.session.awaitingReplacement(), true,
                "forced replacement waits after faint playback");
    test.assert(replacement.submit({MenuIntentKind::Back, 0}), false,
                "forced replacement rejects Back");
    test.assert(replacement.submit({MenuIntentKind::SelectParty, 3}), false,
                "forced replacement rejects out-of-range slot");
    test.assert(replacement.submit({MenuIntentKind::SelectParty, 0}), false,
                "forced replacement rejects dead active slot");
    test.assert(replacement.submit({MenuIntentKind::SelectParty, 1}), true,
                "forced replacement accepts live original party slot");
    replacement.drain(true);
    test.assert(replacement.session.awaitingPlayer() || replacement.session.exitReady(), true,
                "replacement playback returns to choice or terminal");

    // Terminal feedback owns the result until completion; HP sync precedes
    // the caller's eventual mode-union destruction.
    preparePlayer();
    Driver terminal;
    terminal.session.beginWild(4, 1, true, 1);
    terminal.session.stateForTest().active[0].hp = 37;
    test.assert(terminal.submit({MenuIntentKind::Escape, 0}), true,
                "wild escape submits terminal action");
    terminal.frame(false);
    test.assert(terminal.session.isActive(), true,
                "terminal session remains resident during feedback");
    test.assert(terminal.session.exitReady(), false,
                "terminal feedback is not acknowledged at start");
    terminal.drain(false);
    test.assert(terminal.session.exitReady(), true,
                "terminal feedback acknowledges before external mode exit");
    test.assert(terminal.session.isActive(), true,
                "terminal result remains resident until mode exit");
    test.assert(player.creatureHPs[0], static_cast<uint8_t>(37),
                "terminal acknowledgement synchronizes persistent active HP");

    suite.addTest(test);
}

inline void BattlePlaybackSuite(TestRunner &runner)
{
    TestSuite suite("Battle playback integration");
    BattlePlaybackIntegrationTest(suite);
    runner.addTestSuite(suite);
}
