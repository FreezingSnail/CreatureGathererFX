#pragma once
#include <stdint.h>

namespace ArenaDemoFixture {
const uint8_t player_blitz[] = { 2, 31, 18, 4, 31, 4, 7, 31, 2 };
const uint8_t player_bulwark[] = { 14, 31, 5, 5, 31, 12, 22, 31, 3 };
const uint8_t player_utility[] = { 9, 31, 1, 12, 31, 16, 13, 31, 10 };
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
