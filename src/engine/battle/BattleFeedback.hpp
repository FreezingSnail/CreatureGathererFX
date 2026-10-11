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
