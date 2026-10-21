#include "Audio.hpp"

// The whole translation unit is absent from AUDIO=0 and host TEST builds.
#if CGFX_AUDIO && !defined(TEST)

#include <ArduboyFX.h>

// The vendored engine is C; both headers are C++-safe but only atm_fx_compact.h
// supplies its own extern "C" block.
extern "C" {
#include "atm/atm_synth.h"
#include "atm/atm_fx_compact.h"
}

#include "MusicCatalog.hpp"

// Platform byte source for the compact decoder. Offsets are data-section
// relative; the FX library adds the data page exactly once. Called from the
// foreground service only, never from the timer.
extern "C" void atm_fx_compact_fetch(uint32_t offset, uint8_t *dst, uint8_t n) {
    FX::readDataBytes(static_cast<uint24_t>(offset), dst, n);
}

namespace audio {

void begin() {
    atm_synth_setup();
    atm_fx_compact_play(kMusicTracks[MusicTrack_Overworld].address,
                        kMusicTracks[MusicTrack_Overworld].length);
}

void service() { atm_fx_compact_service(); }

void pause() { atm_fx_compact_pause(); }

void resume() { atm_fx_compact_resume(); }

void stop() { atm_fx_compact_stop(); }

bool isPaused() { return atm_fx_compact_is_paused() != 0; }

} // namespace audio

#endif // CGFX_AUDIO && !TEST
