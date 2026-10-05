#pragma once
#include <stdint.h>

namespace ArenaDemoFixture {
const uint8_t player_blitz[] = { 2, 31, 8, 9, 10, 26, 4, 31, 12, 13, 14, 32, 7, 31, 4, 5, 6, 32 };
const uint8_t player_bulwark[] = { 14, 31, 12, 2, 41, 13, 5, 31, 13, 20, 21, 12, 22, 31, 2, 28, 4, 30 };
const uint8_t player_utility[] = { 9, 31, 2, 37, 32, 32, 12, 31, 27, 43, 24, 32, 13, 31, 11, 23, 39, 10 };
constexpr uint8_t trainer_starter = 19;
const uint8_t opponent_starter[] = { 3, 6, 0 };
constexpr uint8_t trainer_speed = 20;
const uint8_t opponent_speed[] = { 18, 16, 28 };
constexpr uint8_t trainer_fortress = 21;
const uint8_t opponent_fortress[] = { 14, 5, 8 };
constexpr uint8_t trainer_tricks = 22;
const uint8_t opponent_tricks[] = { 9, 12, 13 };
constexpr uint8_t trainer_champion = 23;
const uint8_t opponent_champion[] = { 31, 29, 30 };
} // namespace ArenaDemoFixture
