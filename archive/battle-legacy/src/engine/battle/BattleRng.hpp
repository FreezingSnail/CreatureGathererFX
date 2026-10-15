#pragma once

#include <stdint.h>

namespace battle {

// A bounded draw is supplied by the caller, keeping scripted state out of
// resident battle state. The callback returns a value in [0, bound).
using RngFn = uint8_t (*)(uint8_t bound);
using RngCallback = RngFn;

struct Rng {
    RngFn next;

    uint8_t roll(uint8_t bound) const
    {
        return next == nullptr || bound == 0 ? 0 : next(bound);
    }
};

#ifdef __AVR__
static_assert(sizeof(Rng) == 2, "battle RNG seam must remain one AVR pointer");
#endif

} // namespace battle
