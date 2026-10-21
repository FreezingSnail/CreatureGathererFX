#pragma once

// Creature Gatherer theme streams packaged as raw FX data entries.
// 0xC1 streaming-loop compact streams (ring 64); see data/music/README.md.
// Device-only: the addresses come from the generated src/fxdata.h symbols.

#include <stdint.h>

#include "../fxdata.h"

enum MusicTrack : uint8_t {
    MusicTrack_Overworld,
    MusicTrack_Combat,
    MusicTrack_Victory,
    MusicTrack_Defeat,
    MusicTrack_Opening,
    MusicTrack_Village,
    MusicTrack_Forest,
    MusicTrack_Coast,
    MusicTrack_Marsh,
    MusicTrack_Ruins,
    MusicTrack_Exploration,
    MusicTrack_Count,
};

struct MusicTrackEntry {
    uint24_t address;
    uint16_t length;
};

constexpr MusicTrackEntry kMusicTracks[MusicTrack_Count] = {
    {music_overworld, 6328},
    {music_combat, 5772},
    {music_victory, 1338},
    {music_defeat, 674},
    {music_opening, 7208},
    {music_village, 1660},
    {music_forest, 6204},
    {music_coast, 2964},
    {music_marsh, 1452},
    {music_ruins, 7056},
    {music_exploration, 1928},
};
