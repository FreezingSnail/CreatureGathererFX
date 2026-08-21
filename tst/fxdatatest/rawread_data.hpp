#pragma once

#include "src/lib/DataTypes.hpp"

constexpr uint16_t rawReadMoveTableFirst = 0x0005;
constexpr uint16_t rawReadMoveTableNext = 0xFFFF;

constexpr OpponentSeed rawReadOpponentSeed = {
    { 0, 31, 0xFFFFFF00UL },
    { 3, 31, 0xFFFFFF00UL },
    { 6, 31, 0xFFFFFF00UL },
};

constexpr uint16_t rawReadCursorFirstPage = 0;
constexpr uint16_t rawReadCursorLastPage = UINT16_MAX;
