#pragma once
#include <stdint.h>

namespace DuelFixture {
constexpr uint8_t typeCount = 5;
constexpr uint8_t noType = 255;
constexpr uint8_t moveCount = 45;
constexpr uint8_t speciesCount = 64;
constexpr uint8_t trainerCount = 24;
constexpr uint8_t trainerOverrideCount = 1;
constexpr uint8_t teamCount = 5;
const uint8_t duelMoveTypes[] = { 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 0, 0, 0, 0, 255, 1, 3, 255, 1, 255, 255, 255, 255, 255, 255, 255, 1 };
const uint8_t duelSpeciesTypes[] = { 1, 1, 255, 1, 1, 255, 2, 1, 4, 1, 2, 255, 1, 2, 255, 2, 2, 3, 1, 1, 255, 1, 1, 255, 2, 1, 3, 1, 0, 255, 1, 3, 255, 1, 3, 255, 1, 4, 255, 1, 1, 255, 1, 2, 255, 1, 2, 255, 2, 2, 3, 1, 1, 255, 2, 1, 4, 1, 1, 255, 2, 1, 3, 1, 0, 255, 1, 0, 255, 1, 3, 255, 2, 3, 1, 1, 4, 255, 2, 4, 2, 1, 3, 255, 2, 3, 1, 2, 0, 4, 2, 0, 3, 2, 0, 3, 1, 4, 255, 2, 4, 2, 2, 4, 2, 2, 4, 1, 2, 4, 1, 2, 4, 0, 2, 4, 0, 2, 2, 4, 2, 1, 2, 2, 1, 0, 2, 1, 2, 2, 2, 0, 2, 1, 2, 2, 1, 0, 2, 1, 2, 2, 1, 0, 2, 3, 2, 2, 3, 2, 2, 3, 2, 2, 2, 3, 2, 2, 3, 2, 1, 4, 2, 1, 4, 2, 3, 4, 2, 0, 4, 2, 0, 4, 2, 0, 2, 1, 0, 255, 2, 4, 0, 2, 0, 4, 2, 0, 4, 1, 0, 255 };
const uint8_t duelTrainerPersona[] = { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3 };
const uint8_t duelTrainerRate[] = { 25, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 50, 50, 50 };
const uint8_t duelDemoPersona[] = { 1 };
const uint8_t duelDemoRate[] = { 45 };
const uint8_t duelTrainerOverrides[] = { 23, 0, 1, 3, 255 };
const uint8_t duelTeamMemberCounts[] = { 3, 3, 3, 3, 3 };
const uint8_t duelTeamMembers[] = { 6, 8, 1, 1, 255, 3, 8, 1, 2, 255, 12, 8, 1, 4, 255, 1, 8, 1, 1, 255, 7, 8, 1, 1, 255, 4, 8, 1, 2, 255, 9, 8, 1, 0, 255, 48, 8, 1, 3, 255, 13, 8, 1, 1, 255, 29, 8, 1, 0, 255, 63, 8, 1, 1, 255, 14, 8, 1, 2, 255, 3, 8, 1, 2, 255, 14, 8, 1, 2, 255, 43, 8, 1, 2, 255 };
constexpr uint8_t teamSlots = 3;
constexpr uint8_t team_balanced = 0;
constexpr uint8_t team_anchors = 1;
constexpr uint8_t team_support = 2;
constexpr uint8_t team_retyped = 3;
constexpr uint8_t team_trap = 4;
} // namespace DuelFixture
