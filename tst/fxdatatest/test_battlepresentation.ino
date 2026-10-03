#include "harness/fx_globals.hpp"
#include "battlepresentation_test.hpp"

void setup() {
    fxTestSetup();
    FxTest test;
    test_battlepresentation(test);
    test.report(F("test_battlepresentation"));
#ifdef CGFX_BATTLE_PRESENTATION_VISUAL
    arduboy.setFrameRate(52);
#endif
}

void loop() {
#ifdef CGFX_BATTLE_PRESENTATION_VISUAL
    // Optional interactive fixture after automated P/F. The same resident
    // result is reused only after playback completes, alternating ordinary/KO.
    static battle::ActionResult result{};
    static battle::BattlePresenter presenter;
    static bool begun = false, knockout = false;
    if (!arduboy.nextFrame()) return;
    arduboy.pollButtons();
    FxReadCounter::resetFrame();
    if (!begun || presenter.done()) {
        battle_presentation_fixture::fill(result, knockout);
        presenter.begin(result);
        knockout = !knockout;
        begun = true;
    } else {
        presenter.update(arduboy.justPressed(A_BUTTON));
    }
    battle::BattleView view = battle_presentation_fixture::afterView(result);
    presenter.overlay(view);
    arduboy.clear();
    battle_presentation_fixture::drawView(view);
    presenter.draw();
    arduboy.display();
#else
    exit(0);
#endif
}
