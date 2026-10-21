#include "Audio.hpp"

// The whole translation unit is absent from AUDIO=0 and host TEST builds.
#if CGFX_AUDIO && !defined(TEST)

#include <ArduboyFX.h>
#include <avr/pgmspace.h>

// The vendored engine is C; both headers are C++-safe but only atm_fx_compact.h
// supplies its own extern "C" block.
extern "C" {
#include "atm/atm_synth.h"
#include "atm/atm_fx_compact.h"
}

#include "MusicCatalog.hpp"
#include "../lib/FxReadCounter.hpp"

// Platform byte source for the compact decoder. Offsets are data-section
// relative; the FX library adds the data page exactly once. Called from the
// foreground service only, never from the timer.
extern "C" void atm_fx_compact_fetch(uint32_t offset, uint8_t *dst, uint8_t n) {
    // Music reads are accounted separately so they can never consume the
    // metadata update/render budget.
    FxReadCounter::recordAudio();
    FX::readDataBytes(static_cast<uint24_t>(offset), dst, n);
}

namespace audio {
namespace {

// Four bytes of routing state. The pause flag lives in the decoder, so the
// countdown asks it directly instead of duplicating it here.
MusicTrack current = MusicTrack_Overworld;
uint16_t resultFrames = 0;
bool ready = false;

} // namespace

void begin() {
    atm_synth_setup();
    ready = true;
    playTrack(MusicTrack_Overworld);
}

void service() {
    atm_fx_compact_service();
    if (resultFrames == 0 || atm_fx_compact_is_paused()) return;
    if (--resultFrames == 0) playTrack(MusicTrack_Overworld);
}

void pause() { atm_fx_compact_pause(); }

void resume() { atm_fx_compact_resume(); }

void stop() {
    ready = false;
    resultFrames = 0;
    atm_fx_compact_stop();
}

bool isPaused() { return atm_fx_compact_is_paused() != 0; }

void playTrack(MusicTrack track) {
    if (!ready) return;
    resultFrames = 0;
    current = track;
    MusicTrackEntry entry;
    memcpy_P(&entry, &kMusicTracks[track], sizeof entry);
    atm_fx_compact_play(entry.address, entry.length);
}

void playResult(MusicTrack track, uint16_t frames) {
    if (!ready) return;
    playTrack(track);
    resultFrames = frames;
}

MusicTrack currentTrack() { return current; }

} // namespace audio

#endif // CGFX_AUDIO && !TEST
