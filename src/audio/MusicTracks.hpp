#pragma once

// Theme identifiers shared by the music catalog and the runtime facade.
// Host-safe on purpose: Audio.hpp takes MusicTrack parameters in every build,
// including the AUDIO=0 no-op stubs, so this header must not pull in the
// device-only generated FX addresses.

#include <stdint.h>

// `overworld` is the current world theme. `opening/village/forest/coast/marsh/
// ruins/exploration` are packaged entries with no area state yet; they exist so
// future areas can select a theme without repacking the FX image.
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

// Result dwell in frames at 52 fps: ~2 s of victory/defeat before the world
// theme returns.
constexpr uint16_t kResultCueFrames = 104;
