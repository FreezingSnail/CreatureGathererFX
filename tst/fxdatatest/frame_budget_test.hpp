#pragma once
// Shared worst-case frame-time assertion for the interactive device suites
// (CreatureGathererFX-dqxm). It reuses the instrumentation first proven by
// test_duel_resources: wrap one shipping frame (update + render + display) in
// micros(), keep the worst observation, and assert it stays under the 52 fps
// budget. This header introduces no new timer: it is the same micros() pattern,
// factored out so the presentation, world-step and save-chain suites can share
// it instead of repeating the literal.

#include "fxtest.hpp"

namespace frame_budget_test_detail {

// 52 fps shipping budget: 1e6 / 52 us rounded down, the value test_duel_resources
// already asserts. A regression that pushes any covered frame over this fails.
constexpr uint32_t FRAME_BUDGET_US = 19230;

// Deliberate negative control. Spins on micros() (not a fixed loop count) so the
// injected slowdown reliably exceeds the budget on the 16 MHz device regardless
// of compiler optimisation, proving the budget assertion is non-vacuous.
inline void deliberateSlowFrame()
{
    const uint32_t start = micros();
    volatile uint8_t sink = 0;
    while (static_cast<uint32_t>(micros() - start) < 25000u)
        sink = static_cast<uint8_t>(sink + 1u);
}

// Per-suite frame timer. Instantiate as a local so no static RAM is added to the
// device suite; the test builds share the 2160 B RAM budget.
struct FrameBudget {
    uint32_t worst = 0;
    uint32_t start = 0;
    uint32_t slowValue = 0;
    bool slowNext = false;

    inline void reset() { worst = 0; slowValue = 0; slowNext = false; }

    // Arm one frame to run the deliberate negative-control slowdown. That frame
    // is recorded in slowValue (never in worst), so a single pass measures both
    // the natural worst and the slowed control without a second frame loop.
    inline void armSlowFrame() { slowNext = true; }
    inline void begin() { start = micros(); }
    inline void end()
    {
        if (slowNext) {
            slowNext = false;
            deliberateSlowFrame();
            slowValue = static_cast<uint32_t>(micros() - start);
            return;
        }
        const uint32_t dt = static_cast<uint32_t>(micros() - start);
        if (dt > worst) worst = dt;
    }

    // The suite's pass condition: the worst natural frame stays within budget.
    inline void expectWithinBudget(FxTest &test,
                                   const __FlashStringHelper *label) const
    {
        Serial.print(F("worst frame us="));
        Serial.println(static_cast<unsigned int>(worst));
        test.expectEq(worst <= FRAME_BUDGET_US, true, label);
    }

    // The permanent negative control: the deliberately slowed frame must be
    // rejected by the same budget. Non-vacuity proof, not a one-off.
    inline void expectDetectsSlowFrame(FxTest &test,
                                       const __FlashStringHelper *label) const
    {
        test.expectEq(slowValue > FRAME_BUDGET_US, true, label);
    }
};

} // namespace frame_budget_test_detail
