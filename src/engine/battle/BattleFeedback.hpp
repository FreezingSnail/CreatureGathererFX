#pragma once

#include "ActionResult.hpp"
#include "BattleState.hpp"
#include "../../lib/uint24.h"
#ifdef __AVR__
#include "../../fxdata.h"
#endif
#include <stdint.h>

namespace battle {

namespace BattleFeedbackText {
constexpr uint8_t passWidth = 25;
#ifdef __AVR__
inline uint24_t passAddress() { return battlePass; }
#else
inline uint24_t passAddress() { return 0; }
#endif
}

// Single-phase footer pacing at the shipping 52 FPS. The natural hold keeps a
// caption readable; a fresh A edge may finish it after the minimum dwell.
enum : uint8_t { FEEDBACK_HOLD_TICKS = 42, FEEDBACK_MIN_DWELL_TICKS = 10 };

// Cached, single-phase feedback footer. Metadata is prepared once at entry.
class BattleFeedback {
  public:
    void begin(const ActionResult &result, const BattleState &state);
    void update(bool freshA);
    bool done() const;
    void draw() const;
    void reset();

  private:
    uint24_t lines_[2];
    uint8_t widths_[2];
    uint8_t elapsed_;
    uint8_t phase_;
};

#ifdef __AVR__
static_assert(sizeof(BattleFeedback) == 10, "battle feedback must remain 10 bytes");
#endif

} // namespace battle
