#pragma once

// Creature Gatherer theme streams packaged as raw FX data entries.
// 0xC1 streaming-loop compact streams (ring 64); see data/music/README.md.
// Device-only: the addresses come from the generated src/fxdata.h symbols.

#include <stdint.h>

#include "../fxdata.h"
#include "MusicTracks.hpp"

// Game-state routing (implemented by the callers; no table is needed because
// each state names one track):
//   world / battle exit        -> MusicTrack_Overworld
//   battle entry               -> MusicTrack_Combat
//   Win / Gathered result      -> MusicTrack_Victory for kResultCueFrames
//   Lose / Draw result         -> MusicTrack_Defeat  for kResultCueFrames
// The cue then falls back to MusicTrack_Overworld on its own. The remaining
// catalog entries (opening/village/forest/coast/marsh/ruins/exploration) are
// packed for future areas; no area state selects them yet.
//
// The table lives in PROGMEM: it is read once per theme change, never per frame,
// and 55 B of static RAM is worth more than the small flash cost of the read.
struct MusicTrackEntry {
    uint24_t address;
    uint16_t length;
};

const MusicTrackEntry kMusicTracks[MusicTrack_Count] PROGMEM = {
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
