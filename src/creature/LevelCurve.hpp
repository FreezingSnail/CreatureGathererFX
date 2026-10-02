#pragma once

#include <stdint.h>

#ifdef __AVR__
#include <avr/pgmspace.h>
#define LEVEL_CURVE_STORAGE PROGMEM
#else
#define LEVEL_CURVE_STORAGE
#endif

constexpr uint8_t LEVEL_COUNT = 32;

// Cumulative experience required for each level. Level one is the minimum;
// entries zero and one both map to level one.
static constexpr uint16_t LEVEL_EXP[LEVEL_COUNT] LEVEL_CURVE_STORAGE = {
    0, 1, 8, 27, 64, 125, 216, 343,
    512, 729, 1000, 1331, 1728, 2197, 2744, 3375,
    4096, 4913, 5832, 6859, 8000, 9261, 10648, 12167,
    13824, 15625, 17576, 19683, 21952, 24389, 27000, 29791,
};

#undef LEVEL_CURVE_STORAGE

static_assert(sizeof(LEVEL_EXP) == 64, "The level curve must use 64 bytes");
static_assert(LEVEL_EXP[31] < 65535, "Maximum cumulative experience must fit in two bytes");

inline uint16_t levelExpThreshold(uint8_t level) {
#ifdef __AVR__
    return pgm_read_word(&LEVEL_EXP[level]);
#else
    return LEVEL_EXP[level];
#endif
}

inline uint8_t levelFromExp(uint16_t exp) {
    uint8_t low = 1;
    uint8_t high = LEVEL_COUNT - 1;
    while (low < high) {
        const uint8_t middle = (low + high + 1) / 2;
        if (exp >= levelExpThreshold(middle)) {
            low = middle;
        } else {
            high = middle - 1;
        }
    }
    return low;
}
