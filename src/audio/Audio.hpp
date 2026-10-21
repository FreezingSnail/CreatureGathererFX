#pragma once

// Music runtime facade. With CGFX_AUDIO=0 (the default) or a host TEST build,
// every entry point is an empty inline call: no audio headers, no AVR
// dependency and no program-flash cost.
#include "MusicConfig.hpp"

#if CGFX_AUDIO && !defined(TEST)

namespace audio {
void begin();
void service();
void pause();
void resume();
void stop();
bool isPaused();
} // namespace audio

#else

namespace audio {
inline void begin() {}
inline void service() {}
inline void pause() {}
inline void resume() {}
inline void stop() {}
inline bool isPaused() { return false; }
} // namespace audio

#endif
