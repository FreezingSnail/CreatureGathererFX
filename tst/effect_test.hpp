#pragma once
#include "test.hpp"
#include "../src/lib/Effect.hpp"

void isStatEffectTest(TestSuite &t) {
    Test test = Test(__func__);

    test.assert(isStatEffect(Effect::NONE), false, "NONE is not a stat effect");
    test.assert(isStatEffect(Effect::ATKUP), true, "ATKUP is a stat effect");
    test.assert(isStatEffect(Effect::DEFUP), true, "DEFUP is a stat effect");
    test.assert(isStatEffect(Effect::ATKDWN), true, "ATKDWN is a stat effect");
    test.assert(isStatEffect(Effect::DEFDWN), true, "DEFDWN is a stat effect");
    test.assert(isStatEffect(Effect::AIRSWPT), false, "AIRSWPT is not a stat effect");
    test.assert(isStatEffect(Effect::BUFTD), false, "BUFTD is not a stat effect");
    t.addTest(test);
}

void EffectSuite(TestRunner &r) {
    TestSuite t = TestSuite("Effect Suite");
    isStatEffectTest(t);
    r.addTestSuite(t);
}
