#pragma once
#include "src/lib/DataTypes.hpp"

// PROGMEM fixture; copy each OpponentSeed with memcpy_P before comparing.
const OpponentSeed opponentSeeds[] PROGMEM = {
    {
        { 0, 31, 0xFFFFFF00UL },
        { 3, 31, 0xFFFFFF00UL },
        { 6, 31, 0xFFFFFF00UL },
    },
    {
        { 1, 31, 0xFFFFFF00UL },
        { 4, 31, 0xFFFFFF00UL },
        { 7, 31, 0xFFFFFF00UL },
    },
    {
        { 2, 31, 0xFFFFFF00UL },
        { 5, 31, 0xFFFFFF00UL },
        { 8, 31, 0xFFFFFF00UL },
    },
    {
        { 0, 31, 0xFFFFFF00UL },
        { 1, 31, 0xFFFFFF00UL },
        { 2, 31, 0xFFFFFF00UL },
    },
    {
        { 3, 31, 0xFFFFFF00UL },
        { 4, 31, 0xFFFFFF00UL },
        { 5, 31, 0xFFFFFF00UL },
    },
    {
        { 6, 31, 0xFFFFFF00UL },
        { 7, 31, 0xFFFFFF00UL },
        { 8, 31, 0xFFFFFF00UL },
    },
    {
        { 11, 31, 0xFFFFFF00UL },
        { 23, 31, 0xFFFFFF00UL },
        { 24, 31, 0xFFFFFF00UL },
    },
    {
        { 10, 31, 0xFFFFFF00UL },
        { 27, 31, 0xFFFFFF00UL },
        { 28, 31, 0xFFFFFF00UL },
    },
    {
        { 12, 31, 0xFFFFFF00UL },
        { 25, 31, 0xFFFFFF00UL },
        { 26, 31, 0xFFFFFF00UL },
    },
    {
        { 13, 31, 0xFFFFFF00UL },
        { 19, 31, 0xFFFFFF00UL },
        { 20, 31, 0xFFFFFF00UL },
    },
    {
        { 14, 31, 0xFFFFFF00UL },
        { 15, 31, 0xFFFFFF00UL },
        { 16, 31, 0xFFFFFF00UL },
    },
    {
        { 16, 31, 0xFFFFFF00UL },
        { 18, 31, 0xFFFFFF00UL },
        { 20, 31, 0xFFFFFF00UL },
    },
    {
        { 24, 31, 0xFFFFFF00UL },
        { 26, 31, 0xFFFFFF00UL },
        { 28, 31, 0xFFFFFF00UL },
    },
    {
        { 8, 31, 0xFFFFFF00UL },
        { 18, 31, 0xFFFFFF00UL },
        { 24, 31, 0xFFFFFF00UL },
    },
    {
        { 5, 31, 0xFFFFFF00UL },
        { 16, 31, 0xFFFFFF00UL },
        { 26, 31, 0xFFFFFF00UL },
    },
    {
        { 2, 31, 0xFFFFFF00UL },
        { 20, 31, 0xFFFFFF00UL },
        { 28, 31, 0xFFFFFF00UL },
    },
    {
        { 9, 31, 0xFFFFFF00UL },
        { 21, 31, 0xFFFFFF00UL },
        { 22, 31, 0xFFFFFF00UL },
    },
    {
        { 18, 31, 0xFFFFFF00UL },
        { 18, 31, 0xFFFFFF00UL },
        { 18, 31, 0xFFFFFF00UL },
    },
    {
        { 29, 31, 0xFFFFFF00UL },
        { 30, 31, 0xFFFFFF00UL },
        { 31, 31, 0xFFFFFF00UL },
    },
};
constexpr uint8_t opponentSeedCount = 19;
