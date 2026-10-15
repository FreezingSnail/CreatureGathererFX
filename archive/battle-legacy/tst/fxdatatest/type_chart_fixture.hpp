#pragma once

#include <stdint.h>
#ifdef __AVR__
#include <avr/pgmspace.h>
#define TYPE_CHART_FIXTURE_STORAGE PROGMEM
#else
#define TYPE_CHART_FIXTURE_STORAGE
#endif

namespace type_chart_fixture {

// Modifier's stable underlying order: None, Quarter, Half, Same, Double,
// Quadruple. These 8x8 gameplay cells mirror the shipping Type.hpp baseline;
// Type::STATUS is an effect marker, not a row/column in this chart.
static const uint8_t expected[8][8] TYPE_CHART_FIXTURE_STORAGE = {
    {3, 3, 3, 3, 3, 3, 3, 0},
    {3, 3, 4, 2, 4, 3, 2, 2},
    {3, 3, 3, 4, 2, 2, 4, 2},
    {3, 4, 2, 3, 3, 4, 2, 2},
    {3, 0, 4, 2, 3, 4, 4, 3},
    {3, 4, 4, 2, 3, 4, 4, 3},
    {3, 0, 4, 2, 3, 4, 4, 3},
    {3, 0, 4, 2, 3, 4, 4, 3},
};

inline uint8_t cell(uint8_t attack, uint8_t defend)
{
#ifdef __AVR__
    return pgm_read_byte(&expected[attack][defend]);
#else
    return expected[attack][defend];
#endif
}

inline uint8_t combined(uint8_t first, uint8_t second)
{
    if (first == 0 || second == 0) return 0;
    int8_t exponent = static_cast<int8_t>(first) +
                      static_cast<int8_t>(second) - 6;
    if (exponent < -2) exponent = -2;
    if (exponent > 2) exponent = 2;
    return static_cast<uint8_t>(exponent + 3);
}

} // namespace type_chart_fixture

#undef TYPE_CHART_FIXTURE_STORAGE
