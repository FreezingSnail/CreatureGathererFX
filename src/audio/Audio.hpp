#pragma once

// Music runtime facade. With CGFX_AUDIO=0 (the default) or a host TEST build,
// every entry point is an empty inline call: no audio headers, no AVR
// dependency and no program-flash cost.
#include "MusicConfig.hpp"
#include "MusicTracks.hpp"

#if CGFX_AUDIO && !defined(TEST)

namespace audio {
void begin();
void service();
void pause();
void resume();
void stop();
bool isPaused();
// No-op until begin() has run, so state routing calls placed in ModeState are
// harmless during setup.
void playTrack(MusicTrack track);
// Play a bounded result cue; service() counts it down and returns to
// MusicTrack_Overworld at zero.
void playResult(MusicTrack track, uint16_t frames);
MusicTrack currentTrack();
} // namespace audio

#else

namespace audio {
inline void begin() {}
inline void service() {}
inline void pause() {}
inline void resume() {}
inline void stop() {}
inline bool isPaused() { return false; }
inline void playTrack(MusicTrack) {}
inline void playResult(MusicTrack, uint16_t) {}
inline MusicTrack currentTrack() { return MusicTrack_Overworld; }
} // namespace audio

#endif
