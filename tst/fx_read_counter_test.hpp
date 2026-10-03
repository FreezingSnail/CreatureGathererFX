#pragma once

#include "test.hpp"
#include "../src/lib/FxReadCounter.hpp"

void FxReadCounterTest(TestSuite &suite) {
    Test test(__func__);
    FxReadCounter::resetFrame();
    test.assert(FxReadCounter::count(), static_cast<uint8_t>(0), "frame reset");
    test.assert(FxReadCounter::markUpdate(), true, "idle update has zero reads");

#if defined(FX_READ_COUNTER) || defined(DEBUG)
    FxReadCounter::record();
    test.assert(FxReadCounter::count(), static_cast<uint8_t>(1), "one logical read");
    FxReadCounter::resetFrame();
    for (uint8_t row = 0; row < 4; ++row) FxReadCounter::record();
    test.assert(FxReadCounter::renderExact(4), true, "four-row render budget");
    test.assert(FxReadCounter::renderExact(5), false, "four is not five");
    FxReadCounter::record();
    test.assert(FxReadCounter::renderExact(4), false, "extra fifth read fails exact four");
    test.assert(FxReadCounter::renderExact(5), true, "five-row render budget");
    test.assert(FxReadCounter::framePassed(), false, "render failure stays visible");

    FxReadCounter::resetFrame();
    FxReadCounter::record();
    test.assert(FxReadCounter::markUpdate(), false, "update read fails zero budget");
    test.assert(FxReadCounter::updateExactZero(), false, "update failure is retained");

    FxReadCounter::resetFrame();
    FxReadCounter::record();
    test.assert(FxReadCounter::transitionExact(1), true, "one-read transition accepted");
    test.assert(FxReadCounter::markUpdate(), true, "transition is outside steady update");
    for (uint8_t row = 0; row < 4; ++row) FxReadCounter::record();
    test.assert(FxReadCounter::renderExact(4), true, "render budget follows transition");
    test.assert(FxReadCounter::framePassed(), true, "valid frame stays valid");

    FxReadCounter::resetFrame();
    FxReadCounter::record();
    test.assert(FxReadCounter::transitionExact(2), false, "wrong transition budget fails");
    test.assert(FxReadCounter::markUpdate(), false, "failed transition remains visible");
    FxReadCounter::resetFrame();
    test.assert(FxReadCounter::updateExactZero(), true, "reset clears update failure");
#endif
    suite.addTest(test);
}

void FxReadCounterSuite(TestRunner &runner) {
    TestSuite suite("FX read counter suite");
    FxReadCounterTest(suite);
    runner.addTestSuite(suite);
}
