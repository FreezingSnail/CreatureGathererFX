#pragma once

#include <stdint.h>

// One count is one logical program-data lookup. Save flash and raster SPI
// streaming have separate ownership and are deliberately outside this budget.
namespace FxReadCounter {

#if defined(FX_READ_COUNTER) || defined(DEBUG) || defined(TEST)
inline uint8_t &frameReads() {
    static uint8_t reads = 0;
    return reads;
}

inline bool &updatePassed() {
    static bool passed = true;
    return passed;
}

inline uint8_t &allowedTransitionReads() {
    static uint8_t reads = 0;
    return reads;
}

inline bool &transitionPassed() {
    static bool passed = true;
    return passed;
}

inline bool &renderPassed() {
    static bool passed = true;
    return passed;
}

inline void resetFrame() {
    frameReads() = 0;
    allowedTransitionReads() = 0;
    updatePassed() = true;
    transitionPassed() = true;
    renderPassed() = true;
}

inline void record() {
    if (frameReads() != UINT8_MAX) ++frameReads();
}

inline uint8_t count() { return frameReads(); }
inline bool exact(uint8_t expected) { return count() == expected; }

// A transition owner calls this immediately after its known lookup sequence.
// The total stays visible while only approved reads leave the steady budget.
inline bool transitionExact(uint8_t expected) {
    const bool passed = count() >= allowedTransitionReads() &&
        static_cast<uint8_t>(count() - allowedTransitionReads()) == expected;
    transitionPassed() = transitionPassed() && passed;
    if (passed) allowedTransitionReads() = count();
    return passed;
}

// Call after run() and before render(). Transition lookups are checked at
// their transition boundary; a steady update performs no program-data reads.
inline bool markUpdate() {
    updatePassed() = updatePassed() && transitionPassed() &&
                     count() == allowedTransitionReads();
    return updatePassed();
}

inline bool updateExactZero() { return updatePassed(); }
inline bool renderExact(uint8_t expected) {
    const bool passed = updatePassed() && transitionPassed() &&
        count() >= allowedTransitionReads() &&
        static_cast<uint8_t>(count() - allowedTransitionReads()) == expected;
    renderPassed() = renderPassed() && passed;
    return passed;
}
inline bool framePassed() {
    return updatePassed() && transitionPassed() && renderPassed();
}
#else
inline void resetFrame() {}
inline void record() {}
inline uint8_t count() { return 0; }
inline bool exact(uint8_t expected) { return expected == 0; }
inline bool transitionExact(uint8_t) { return true; }
inline bool markUpdate() { return true; }
inline bool updateExactZero() { return true; }
inline bool renderExact(uint8_t) { return true; }
inline bool framePassed() { return true; }
#endif

} // namespace FxReadCounter
