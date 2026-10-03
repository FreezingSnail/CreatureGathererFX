#pragma once

#include "../src/engine/battle/BattlePresenter.hpp"
#include "test.hpp"

inline battle::ActionResult presentationHostResult(bool knockout = false) {
    battle::ActionResult result{};
    battle::resetActionResult(result);
    result.kind = battle::ResultKind::Attack;
    result.actor = battle::Side::Player;
    result.index = 0;
    result.speciesBefore[0] = 0;
    result.speciesBefore[1] = 31;
    result.maxHpBefore[0] = result.maxHpBefore[1] = 100;
    result.hpBefore[0] = result.hpAfter[0] = 80;
    result.hpBefore[1] = 60;
    result.hpAfter[1] = knockout ? 0 : 35;
    result.flags = knockout ? battle::OPPONENT_FAINTED : 0;
    return result;
}

inline void presentationTicks(battle::BattlePresenter &presenter, uint16_t ticks) {
    while (ticks-- != 0) presenter.update(false);
}

inline void presentationNatural(battle::BattlePresenter &presenter) {
    for (uint16_t ticks = 0; ticks < 1000 && !presenter.done(); ++ticks)
        presenter.update(false);
}

inline void presentationAccelerated(battle::BattlePresenter &presenter) {
    // Each loop is one newly pressed A edge followed by the rest of the
    // minimum readable dwell. Consecutive facts may share the same enum stage.
    for (uint16_t stages = 0; stages < 100 && !presenter.done(); ++stages) {
        presenter.update(true);
        for (uint8_t tick = 1; tick < battle::MIN_DWELL_TICKS && !presenter.done(); ++tick)
            presenter.update(false);
    }
}

inline battle::BattleView presentationAfterView(const battle::ActionResult &result) {
    battle::BattleView view{};
    for (uint8_t side = 0; side < 2; ++side) {
        view.active[side] = {result.speciesBefore[side], result.hpAfter[side],
                             result.maxHpBefore[side]};
    }
    view.gatherProgress = result.progressAfter;
    return view;
}

inline void BattlePresentationSuite(TestRunner &runner) {
    using namespace battle;
    TestSuite suite("Battle presentation");
    Test test(__func__);
    ActionResult result = presentationHostResult();
    BattlePresenter presenter;
    BattleView view = presentationAfterView(result);

    presenter.begin(result);
    presenter.overlay(view);
    test.assert(presenter.stage(), PresenterStage::Announce, "attack begins with announcement");
    test.assert(view.active[1].hp, 60, "attack begins from historical HP");
    test.assert(view.active[0].id, 0, "species zero is a valid active sprite");
    for (uint8_t tick = 0; tick < ANNOUNCE_TICKS - 1; ++tick) {
        presenter.draw(); presenter.draw();
        view = presentationAfterView(result); presenter.overlay(view);
        test.assert(view.active[1].hp, 60, "draws do not reveal impact early");
        presenter.update(false);
    }
    test.assert(presenter.stage(), PresenterStage::Announce, "last announce tick remains visible");
    presenter.update(false);
    view = presentationAfterView(result); presenter.overlay(view);
    test.assert(presenter.stage(), PresenterStage::Impact, "exact announce boundary enters impact");
    test.assert(view.active[1].hp, 35, "attack HP changes at impact entry");
    test.assert(view.active[0].hp, 80, "unaffected actor keeps current HP");
    presentationTicks(presenter, IMPACT_TICKS);
    test.assert(presenter.done(), true, "ordinary attack completes without input");
    presenter.update(true); presenter.draw();
    view = presentationAfterView(result); presenter.overlay(view);
    test.assert(view.active[1].hp, 35, "completed attack retains final HP");

    // Fresh A shortens each stage once, but held/no-edge updates do nothing.
    ActionResult naturalResult = presentationHostResult(true);
    BattlePresenter natural, accelerated;
    natural.begin(naturalResult);
    accelerated.begin(naturalResult);
    presentationNatural(natural);
    presentationAccelerated(accelerated);
    view = presentationAfterView(naturalResult); natural.overlay(view);
    BattleView fastView = presentationAfterView(naturalResult); accelerated.overlay(fastView);
    test.assert(natural.done() && accelerated.done(), true, "natural and accelerated KO finish");
    test.assert(fastView.active[1].hp, view.active[1].hp, "A acceleration preserves final HP");
    test.assert(fastView.active[1].id, view.active[1].id, "A acceleration preserves hidden sprite");
    test.assert(view.active[1].id, 255, "faint hides only after feedback completes");

    result = presentationHostResult(true);
    presenter.begin(result);
    presentationTicks(presenter, ANNOUNCE_TICKS);
    test.assert(presenter.stage(), PresenterStage::Impact, "KO impact follows announcement");
    presentationTicks(presenter, IMPACT_TICKS);
    test.assert(presenter.stage(), PresenterStage::Faint, "KO has a separate faint stage");
    view = presentationAfterView(result); presenter.overlay(view);
    test.assert(view.active[1].id, 31, "KO sprite remains visible during faint feedback");
    presentationTicks(presenter, FAINT_TICKS - 1);
    test.assert(presenter.stage(), PresenterStage::Faint, "faint remains through its final tick");
    presentationTicks(presenter, 1);
    view = presentationAfterView(result); presenter.overlay(view);
    test.assert(presenter.done(), true, "KO playback completes automatically");
    test.assert(view.active[1].id, 255, "fainted sprite hides at faint completion");

    result = presentationHostResult();
    result.flags = SELF_HIT;
    result.hpAfter[0] = 55;
    result.hpAfter[1] = result.hpBefore[1];
    presenter.begin(result);
    presentationTicks(presenter, ANNOUNCE_TICKS);
    test.assert(presenter.stage(), PresenterStage::Impact, "self-hit reaches impact");
    view = presentationAfterView(result); presenter.overlay(view);
    test.assert(view.active[0].hp, 55, "self-hit applies HP change to its actor");
    test.assert(view.active[1].hp, 60, "self-hit leaves the other side unchanged");
    presentationTicks(presenter, IMPACT_TICKS);
    test.assert(presenter.done(), true, "self-hit playback completes automatically");

    result = presentationHostResult();
    result.actor = Side::Opponent;
    result.hpAfter[0] = 52;
    result.hpAfter[1] = result.hpBefore[1];
    presenter.begin(result);
    presentationTicks(presenter, ANNOUNCE_TICKS);
    view = presentationAfterView(result); presenter.overlay(view);
    test.assert(view.active[0].hp, 52, "opponent attack updates the Player target");
    test.assert(view.active[1].hp, 60, "opponent attack preserves its actor HP");
    presentationTicks(presenter, IMPACT_TICKS);
    test.assert(presenter.done(), true, "opponent attack playback completes");

    resetActionResult(result);
    result.kind = ResultKind::Skip;
    result.actor = Side::Opponent;
    result.flags = STATUS_SKIPPED;
    result.speciesBefore[1] = 31;
    presenter.begin(result);
    test.assert(presenter.stage(), PresenterStage::Consequence,
                "STATUS_SKIPPED prepares automatic feedback");
    presentationTicks(presenter, CONSEQUENCE_TICKS - 1);
    test.assert(presenter.stage(), PresenterStage::Consequence,
                "status-skip feedback remains through its readable dwell");
    presentationTicks(presenter, 1);
    test.assert(presenter.done(), true, "status-skip feedback completes automatically");

    // A late edge cannot extend a stage; an early edge leaves exactly 10 ticks.
    result = presentationHostResult();
    presenter.begin(result);
    presentationTicks(presenter, ANNOUNCE_TICKS - MIN_DWELL_TICKS - 1);
    presenter.update(true);
    test.assert(presenter.stage(), PresenterStage::Announce, "early A cannot skip announce");
    presentationTicks(presenter, MIN_DWELL_TICKS - 1);
    test.assert(presenter.stage(), PresenterStage::Impact, "early A leaves minimum dwell");
    presenter.begin(result);
    presentationTicks(presenter, ANNOUNCE_TICKS - 2);
    presenter.update(true);
    test.assert(presenter.stage(), PresenterStage::Announce, "late A does not jump boundary");
    presenter.update(false);
    test.assert(presenter.stage(), PresenterStage::Impact, "late A does not lengthen stage");

    // Two attack effects play in result order, then Player faint before Opponent.
    result = presentationHostResult(true);
    result.flags = PLAYER_FAINTED | OPPONENT_FAINTED;
    result.hpBefore[0] = 12;
    result.hpAfter[0] = 0;
    result.consequences[0] = {Effect::ATKUP, static_cast<uint8_t>(Side::Player), 4};
    result.consequences[1] = {Effect::SOAKED, static_cast<uint8_t>(Side::Opponent), 0};
    presenter.begin(result);
    presentationTicks(presenter, ANNOUNCE_TICKS + IMPACT_TICKS);
    test.assert(presenter.stage(), PresenterStage::Consequence, "first successful effect follows impact");
    presentationTicks(presenter, CONSEQUENCE_TICKS);
    test.assert(presenter.stage(), PresenterStage::Consequence, "second effect follows first in array order");
    presentationTicks(presenter, CONSEQUENCE_TICKS);
    test.assert(presenter.stage(), PresenterStage::Faint, "effects precede faint feedback");
    view = presentationAfterView(result); presenter.overlay(view);
    test.assert(view.active[0].id, 0, "Player faint is presented first");
    presentationTicks(presenter, FAINT_TICKS);
    view = presentationAfterView(result); presenter.overlay(view);
    test.assert(view.active[0].id, 255, "Player sprite hides after first faint");
    test.assert(view.active[1].id, 31, "Opponent sprite remains until second faint");
    test.assert(presenter.stage(), PresenterStage::Faint, "Opponent faint follows Player faint");
    presentationTicks(presenter, FAINT_TICKS);
    test.assert(presenter.done(), true, "both faint facts complete automatically");

    // EndTurn tick facts preserve each intermediate HP, including net-zero mixed ticks.
    resetActionResult(result);
    result.kind = ResultKind::EndTurn;
    result.outcome = Outcome::Fled;
    result.speciesBefore[0] = 0; result.speciesBefore[1] = 31;
    result.maxHpBefore[0] = result.maxHpBefore[1] = 100;
    result.hpBefore[0] = result.hpBefore[1] = 100;
    result.hpAfter[0] = result.hpAfter[1] = 100;
    result.consequences[0] = {Effect::SAPPD, 0, 90};
    result.consequences[1] = {Effect::INFSED, 0, 100};
    result.consequences[2] = {Effect::SAPPD, 1, 85};
    result.consequences[3] = {Effect::INFSED, 1, 100};
    presenter.begin(result);
    for (uint8_t fact = 0; fact < 4; ++fact) {
        test.assert(presenter.stage(), PresenterStage::Impact, "each end-turn tick has impact stage");
        view = presentationAfterView(result); presenter.overlay(view);
        const uint8_t expectedPlayer[] = {90, 100, 100, 100};
        const uint8_t expectedOpponent[] = {100, 100, 85, 100};
        test.assert(view.active[0].hp, expectedPlayer[fact], "Player tick HP appears in slot order");
        test.assert(view.active[1].hp, expectedOpponent[fact], "Opponent tick HP appears in slot order");
        presentationTicks(presenter, IMPACT_TICKS);
    }
    test.assert(presenter.stage(), PresenterStage::Terminal, "flee feedback follows all tick facts");
    presentationTicks(presenter, TERMINAL_TICKS);
    test.assert(presenter.done(), true, "EndTurn terminal completes automatically");

    // Switch uses the outgoing snapshot until impact, then the fresh caller view supplies maxHP.
    resetActionResult(result);
    result.kind = ResultKind::Switch;
    result.actor = Side::Player;
    result.index = 12;
    result.speciesBefore[0] = 3; result.speciesBefore[1] = 6;
    result.maxHpBefore[0] = 50; result.maxHpBefore[1] = 70;
    result.hpBefore[0] = 20; result.hpBefore[1] = 40;
    result.hpAfter[0] = 80; result.hpAfter[1] = 40;
    BattleView afterSwitch{};
    afterSwitch.active[0] = {12, 80, 120};
    afterSwitch.active[1] = {6, 40, 70};
    presenter.begin(result);
    view = afterSwitch; presenter.overlay(view);
    test.assert(view.active[0].id, 3, "outgoing species remains before switch impact");
    test.assert(view.active[0].maxHp, 50, "outgoing maxHP remains before switch impact");
    test.assert(view.active[0].hp, 20, "outgoing HP remains before switch impact");
    presentationTicks(presenter, ANNOUNCE_TICKS);
    view = afterSwitch; presenter.overlay(view);
    test.assert(view.active[0].id, 12, "incoming species appears at switch impact");
    test.assert(view.active[0].maxHp, 120, "incoming maxHP comes from fresh view at impact");
    test.assert(view.active[0].hp, 80, "incoming HP appears at switch impact");
    presentationTicks(presenter, IMPACT_TICKS);
    test.assert(presenter.done(), true, "voluntary switch completes automatically");

    // Forced replacement begins with an absent defeated sprite, then reveals incoming species.
    result.flags = FORCED_SWITCH;
    result.hpBefore[0] = 0;
    result.hpAfter[0] = 7;
    afterSwitch.active[0] = {12, 7, 120};
    presenter.begin(result);
    view = afterSwitch; presenter.overlay(view);
    test.assert(view.active[0].id, 255, "forced switch keeps defeated sprite hidden before impact");
    presentationTicks(presenter, ANNOUNCE_TICKS);
    view = afterSwitch; presenter.overlay(view);
    test.assert(view.active[0].id, 12, "forced incoming sprite appears at impact");

    // Gather progress has a before/after boundary; refused choices and explicit skips finish too.
    resetActionResult(result);
    result.kind = ResultKind::Gather; result.actor = Side::Player;
    result.speciesBefore[0] = 0; result.maxHpBefore[0] = 100;
    result.hpBefore[0] = result.hpAfter[0] = 80;
    result.progressBefore = 2; result.progressAfter = 5;
    presenter.begin(result);
    view = presentationAfterView(result); presenter.overlay(view);
    test.assert(view.gatherProgress, 2, "Gather keeps old progress before impact");
    presentationTicks(presenter, ANNOUNCE_TICKS);
    view = presentationAfterView(result); presenter.overlay(view);
    test.assert(view.gatherProgress, 5, "Gather progress changes at impact");
    presentationTicks(presenter, IMPACT_TICKS);
    test.assert(presenter.done(), true, "Gather action completes automatically");

    result.flags = REFUSED;
    presenter.begin(result);
    test.assert(presenter.stage(), PresenterStage::Consequence, "refused Gather has readable feedback stage");
    presentationTicks(presenter, CONSEQUENCE_TICKS);
    test.assert(presenter.done(), true, "refused Gather completes automatically");
    resetActionResult(result); result.kind = ResultKind::Escape; result.flags = REFUSED;
    presenter.begin(result);
    presentationTicks(presenter, CONSEQUENCE_TICKS);
    test.assert(presenter.done(), true, "refused Escape completes automatically");
    resetActionResult(result); result.kind = ResultKind::Skip;
    presenter.begin(result);
    presentationTicks(presenter, CONSEQUENCE_TICKS);
    test.assert(presenter.done(), true, "explicit Skip completes automatically");
    resetActionResult(result); result.kind = ResultKind::Escape; result.outcome = Outcome::Escaped;
    presenter.begin(result);
    test.assert(presenter.stage(), PresenterStage::Terminal, "successful Escape starts terminal feedback");
    presentationTicks(presenter, TERMINAL_TICKS);
    test.assert(presenter.done(), true, "successful Escape completes automatically");

    const Outcome terminalOutcomes[] = {Outcome::Win, Outcome::Lose, Outcome::Gathered};
    for (Outcome outcome : terminalOutcomes) {
        resetActionResult(result);
        result.kind = ResultKind::EndTurn;
        result.outcome = outcome;
        result.actor = Side::Player;
        result.speciesBefore[0] = 0;
        presenter.begin(result);
        test.assert(presenter.stage(), PresenterStage::Terminal,
                    "Win/Lose/Gathered outcome starts terminal feedback");
        presentationTicks(presenter, TERMINAL_TICKS);
        test.assert(presenter.done(), true,
                    "Win/Lose/Gathered terminal feedback completes automatically");
    }

    // Sentinel/empty move and absent species remain bounded, and maxHP zero is safe.
    result = presentationHostResult();
    result.index = 255;
    result.maxHpBefore[0] = 0;
    presenter.begin(result);
    view = presentationAfterView(result); presenter.overlay(view);
    test.assert(presenter.stage(), PresenterStage::Announce, "missing move sentinel keeps valid attack stage");
    test.assert(view.active[0].hp, 0, "zero maxHP clamps displayed HP safely");
    result.index = 32;
    presenter.begin(result);
    test.assert(presenter.stage(), PresenterStage::Announce, "empty move ID remains a valid no-name entry");
    result.speciesBefore[0] = 255;
    presenter.begin(result); presenter.draw();
    test.assert(presenter.stage(), PresenterStage::Announce, "absent species name is skipped safely");

    resetActionResult(result);
    presenter.begin(result);
    test.assert(presenter.done(), true, "None result is immediately complete");
    test.assert(presenter.stage(), PresenterStage::Done, "None result enters Done stage");
    presenter.update(true);
    test.assert(presenter.done(), true, "None result stays complete after A input");

    suite.addTest(test);
    runner.addTestSuite(suite);
}
