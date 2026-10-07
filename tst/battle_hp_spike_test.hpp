#pragma once
#include "battle_presentation_test.hpp"

inline void BattleHpSpikeSuite(TestRunner &runner) {
    TestSuite suite("Battle HP research spike");
    Test test("Battle HP research spike");
    using namespace battle;
    for (uint8_t scenario = 0; scenario < 5; ++scenario) {
        ActionResult result = presentationHostResult(false);
        const uint8_t before[] = {60, 1, 255, 60, 60};
        const uint8_t after[] = {35, 0, 0, 60, 35};
        result.maxHpBefore[1] = scenario == 2 ? 255 : 100;
        result.hpBefore[1] = before[scenario];
        result.hpAfter[1] = after[scenario];
        if (scenario == 4) {
            result.flags |= SELF_HIT;
            result.hpBefore[0] = 60; result.hpAfter[0] = 35;
        }
        BattlePresenter presenter;
        presenter.begin(result);
        for (uint8_t i = 0; i < ANNOUNCE_TICKS; ++i) presenter.update(false);
        const uint8_t target = scenario == 4 ? 0 : 1;
        uint8_t previous = result.hpBefore[target];
        for (uint8_t tick = 0; tick < IMPACT_TICKS; ++tick) {
            BattleView view = presentationAfterView(result);
            presenter.overlay(view); presenter.overlayHpSpike(view);
            const uint8_t hp = view.active[target].hp;
            test.assert(hp <= previous, true, "damage is monotonic");
            if (tick <= 6) test.assert(hp, result.hpBefore[target], "reaction holds HP");
            if (tick == 20) test.assert(hp, result.hpAfter[target], "settles before exit");
            previous = hp;
            presenter.update(false);
        }
        test.assert(result.hpAfter[target], after[scenario], "borrowed result is unchanged");
    }
    // Damage and healing cancel overall, but each resident fact remains visible.
    ActionResult ticks{}; resetActionResult(ticks);
    ticks.kind = ResultKind::EndTurn;
    ticks.speciesBefore[0] = ticks.speciesBefore[1] = 0;
    ticks.maxHpBefore[0] = ticks.maxHpBefore[1] = 100;
    ticks.hpBefore[0] = ticks.hpAfter[0] = 80;
    ticks.consequences[0] = {Effect::SAPPD, 0, 60};
    ticks.consequences[1] = {Effect::INFSED, 0, 80};
    BattlePresenter presenter; presenter.begin(ticks);
    for (uint8_t fact = 0; fact < 2; ++fact) {
        for (uint8_t tick = 0; tick < IMPACT_TICKS; ++tick) {
            BattleView view = presentationAfterView(ticks);
            presenter.overlay(view); presenter.overlayHpSpike(view);
            if (tick == 0) test.assert(view.active[0].hp, fact == 0 ? 80 : 60,
                                       "tick starts at preceding endpoint");
            if (tick == 20) test.assert(view.active[0].hp, fact == 0 ? 60 : 80,
                                        "tick settles independently");
            presenter.update(false);
        }
    }
    test.assert(presenter.done(), true, "net-zero ticks finish normally");
    for (uint8_t edge = 0; edge < 2; ++edge) {
        ActionResult invalid = presentationHostResult();
        invalid.maxHpBefore[1] = edge == 0 ? 0 : 10;
        invalid.hpBefore[1] = 255; invalid.hpAfter[1] = 200;
        presenter.begin(invalid); presentationTicks(presenter, ANNOUNCE_TICKS);
        for (uint8_t tick = 0; tick < IMPACT_TICKS; ++tick) {
            BattleView view = presentationAfterView(invalid);
            presenter.overlay(view); presenter.overlayHpSpike(view);
            test.assert(view.active[1].hp, invalid.maxHpBefore[1],
                        "zero max and overflowing endpoints clamp before interpolation");
            presenter.update(false);
        }
    }
    suite.addTest(test);
    runner.addTestSuite(suite);
}
