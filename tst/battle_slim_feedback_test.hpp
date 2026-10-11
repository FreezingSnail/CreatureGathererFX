#pragma once

#include "test.hpp"
#include "../src/engine/battle/BattleFeedback.hpp"
#include "../src/engine/battle/BattleLayout.hpp"
#include "../src/engine/battle/BattleScene.hpp"
#include "../src/lib/FxReadCounter.hpp"
#include "src/FXDataFake.hpp"
#include "../src/fxdata.h"
#include <Arduboy2.h>
#include <cstring>

namespace battle_slim_feedback_fixture {

inline void SlimFeedbackTest(TestSuite &suite)
{
    using namespace battle;
    Test test(__func__);
    battle::BattleState state = {};
    state.active[0].id = 3;
    state.active[0].hp = state.active[0].maxHp = 10;
    state.active[1].id = 4;
    state.active[1].hp = 1;
    state.active[1].maxHp = 10;

    ActionResult result;
    resetActionResult(result);
    result.kind = ResultKind::Attack;
    result.actor = Side::Player;
    result.index = 0;
    result.speciesBefore[0] = 3;

    BattleFeedback feedback;
    FxReadCounter::resetFrame();
    feedback.begin(result, state);
    test.assert(FxReadCounter::count(), static_cast<uint8_t>(2),
                "attack feedback prepares only actor and move addresses");
    FxReadCounter::resetFrame();
    fxDataFake::readCount = 0;
    feedback.draw();
    feedback.update(false);
    test.assert(FxReadCounter::count(), static_cast<uint8_t>(0),
                "feedback drawing and update use cached metadata");
    test.assert(fxDataFake::lastDataAddress > 0x010000u, true,
                "cached creature and move addresses retain their high bytes");
    test.assert(feedback.done(), false, "feedback remains through its hold");
    for (uint8_t tick = 0; tick < 3; ++tick) feedback.update(false);
    test.assert(feedback.done(), false, "A cannot finish before four ticks");
    feedback.update(true);
    test.assert(feedback.done(), true, "fresh A finishes after four ticks");
    feedback.update(false);
    test.assert(feedback.done(), true, "completed feedback stays done");

    result.flags = SELF_HIT;
    feedback.begin(result, state);
    fxDataFake::dataBase = 0;
    feedback.draw();
    test.assert(fxDataFake::lastDataAddress,
                static_cast<uint32_t>(battleHurtItself + 59u),
                "confusion self-hit uses its authored second-row caption");

    result.flags = REFUSED;
    feedback.begin(result, state);
    feedback.draw();
    test.assert(fxDataFake::lastDataAddress,
                static_cast<uint32_t>(battleCannotAct + 54u),
                "refused attack uses the cannot-act caption");

    result.flags = 0;
    result.kind = ResultKind::Switch;
    result.flags = FORCED_SWITCH;
    result.actor = Side::Player;
    state.active[0].id = 5;
    feedback.begin(result, state);
    feedback.draw();
    test.assert(fxDataFake::lastDataAddress,
                static_cast<uint32_t>(battleEnters + 84u),
                "forced replacement uses the enters caption");

    resetActionResult(result);
    result.kind = ResultKind::Skip;
    result.actor = Side::Opponent;
    result.speciesBefore[1] = 4;
    result.outcome = Outcome::Escaped;
    feedback.begin(result, state);
    feedback.draw();
    test.assert(fxDataFake::lastDataAddress,
                static_cast<uint32_t>(battleEscaped + 39u),
                "terminal outcome overrides the action caption");
    for (uint8_t tick = 0; tick < 25; ++tick) feedback.update(false);
    test.assert(feedback.done(), false, "terminal feedback holds for 26 ticks");
    feedback.update(false);
    test.assert(feedback.done(), true, "terminal feedback completes on tick 26");

    feedback.begin(result, state);
    FxReadCounter::resetFrame();
    fxDataFake::readCount = 0;
    feedback.draw();
    test.assert(FxReadCounter::count(), static_cast<uint8_t>(0),
                "caption raster draw does not perform metadata lookups");
    test.assert(fxDataFake::lastDataAddress > 0x010000u, true,
                "caption pixels retain the high 24-bit packed address");

    state.active[0].id = 3;
    std::memset(Arduboy2Base::sBuffer, 0, Arduboy2Base::BUFFER_BYTES);
    fxDataFake::readCount = 0;
    BattleScene::draw(state);
    test.assert(Arduboy2Base::getPixel(50, 36), static_cast<uint8_t>(1),
                "player HP bar shows current HP at low values");
    test.assert(Arduboy2Base::getPixel(50, 11), static_cast<uint8_t>(1),
                "opponent HP bar is drawn from active RAM state");
    const uint24_t playerFrameEnd = battleSprites48 + 4 +
        static_cast<uint24_t>(7u * battle_layout::spriteStride48) + 575u;
    test.assert(fxDataFake::lastDataAddress, playerFrameEnd,
                "scene keeps the back-facing odd frame for the player");

    suite.addTest(test);
}

inline void SlimFeedbackSuite(TestRunner &runner)
{
    TestSuite suite("Slim battle feedback and scene");
    SlimFeedbackTest(suite);
    runner.addTestSuite(suite);
}

} // namespace battle_slim_feedback_fixture
