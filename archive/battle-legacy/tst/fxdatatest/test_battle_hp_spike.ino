#include "harness/fx_globals.hpp"
#include "battlepresentation_test.hpp"

// Paint outside the real presenter/result/view/render call chain.
__attribute__((noinline)) void hpSpikeChain() {
    battle::ActionResult result{};
    battle::BattlePresenter presenter;
    for (uint8_t scenario = 0; scenario < 3; ++scenario) {
        battle_presentation_fixture::fill(result, scenario == 1);
        if (scenario == 2) {
            result.kind = battle::ResultKind::EndTurn;
            result.hpBefore[0] = 40; result.hpAfter[0] = 80;
            result.consequences[0] = {Effect::INFSED, 0, 80};
        }
        presenter.begin(result);
        while (!presenter.done()) {
            auto view = battle_presentation_fixture::afterView(result);
            presenter.overlay(view); presenter.overlayHpSpike(view);
            arduboy.clear();
            battle_presentation_fixture::drawView(view);
            presenter.draw(); presenter.update(false);
        }
    }
}

void setup() {
    fxTestSetup();
    FxTest test;
    {
        battle::ActionResult result{};
        battle::BattlePresenter presenter;
        for (uint8_t scenario = 0; scenario < 4; ++scenario) {
            battle_presentation_fixture::fill(result, scenario == 1);
            if (scenario == 1) result.hpBefore[1] = 1;
            if (scenario >= 2) {
                result.kind = battle::ResultKind::EndTurn;
                result.hpBefore[0] = 40; result.hpAfter[0] = 80;
                result.consequences[0] = {Effect::INFSED, 0, 80};
                if (scenario == 3) {
                    result.hpBefore[0] = result.hpAfter[0] = 80;
                    result.consequences[0] = {Effect::SAPPD, 0, 40};
                    result.consequences[1] = {Effect::INFSED, 0, 80};
                }
            }
            presenter.begin(result);
            if (scenario < 2)
                for (uint8_t i = 0; i < battle::ANNOUNCE_TICKS; ++i) presenter.update(false);
            const uint8_t side = scenario < 2 ? 1 : 0;
            const uint8_t facts = scenario == 3 ? 2 : 1;
            for (uint8_t fact = 0; fact < facts; ++fact) {
                const uint8_t start = fact == 0 ? result.hpBefore[side] : 40;
                const uint8_t goal = scenario < 2 ? result.hpAfter[side]
                    : result.consequences[fact].value;
                uint8_t previous = start;
                for (uint8_t tick = 0; tick < battle::IMPACT_TICKS; ++tick) {
                    auto view = battle_presentation_fixture::afterView(result);
                    FxReadCounter::resetFrame();
                    presenter.overlay(view); presenter.overlayHpSpike(view);
                    test.expectEq(FxReadCounter::count(), 0, F("HP overlay has no metadata reads"));
                    const uint8_t hp = view.active[side].hp;
                    test.expectEq(goal >= start ? hp >= previous : hp <= previous,
                                  true, F("HP movement is monotonic"));
                    if (tick <= 6) test.expectEq(hp, start, F("reaction retains old HP"));
                    if (tick == 20) test.expectEq(hp, goal, F("HP settles before impact exits"));
                    previous = hp; presenter.update(false);
                }
            }
        }
        // Fresh A skips elapsed time, never the final endpoint or faint order.
        battle_presentation_fixture::fill(result, true);
        presenter.begin(result);
        for (uint8_t i = 0; i < 80 && !presenter.done(); ++i) presenter.update(true);
        auto view = battle_presentation_fixture::afterView(result);
        presenter.overlay(view); presenter.overlayHpSpike(view);
        test.expectEq(presenter.done(), true, F("accelerated KO finishes"));
        test.expectEq(view.active[1].hp, 0, F("accelerated KO settles to zero"));
        test.expectEq(view.active[1].id, 255, F("KO hides creature only after faint"));
    }
    const uint16_t top = battle_presentation_test_detail::paint();
    hpSpikeChain();
    const uint16_t headroom = battle_presentation_test_detail::lowWater(top)
        - reinterpret_cast<uint16_t>(&battle_presentation_test_detail::__bss_end);
    Serial.print(F("HP spike painted headroom=")); Serial.println(headroom);
    Serial.print(F("HP spike effective headroom=")); Serial.println(headroom >= 69 ? headroom - 69 : 0);
    test.expectEq(headroom >= 219, true, F("HP spike preserves effective 150 B reserve"));
    test.expectEq(sizeof(battle::BattlePresenter), 24, F("presenter state does not grow"));
    test.report(F("test_battle_hp_spike"));
}

void loop() { exit(0); }
