#pragma once

#include "fxtest.hpp"
#include "src/lib/FxRead.hpp"
#include "src/lib/ReadData.hpp"
#include "src/fxdata.h"
#include "src/engine/draw.h"

#if CGFX_AUDIO
#include "src/audio/Audio.hpp"
#include "src/globals.hpp"
#include "src/save/Compaction.hpp"
#include "src/save/SaveController.hpp"
// The vendored engine is C; atm_synth.h supplies the state structs that
// atm_fx_compact.h requires.
extern "C" {
#include "src/audio/atm/atm_synth.h"
#include "src/audio/atm/atm_fx_compact.h"
}
#endif

namespace readcounter_audio_detail {
#if CGFX_AUDIO
inline void run(FxTest &test) {
    audio::begin();

    bool audioReadsSeen = false;
    bool metadataDirty = false;
    for (uint8_t i = 0; i < 100; ++i) {
        FX::display(CLEAR_BUFFER);
        FxReadCounter::resetFrame();
        audio::service();
        if (FxReadCounter::audioCount() != 0) audioReadsSeen = true;
        if (FxReadCounter::count() != 0) metadataDirty = true;
        delay(20);
    }
    test.expectEq(audioReadsSeen, true, F("audio service reads FX data"));
    test.expectEq(metadataDirty, false, F("audio reads stay out of metadata budget"));

    Serial.print(F("audio ticks="));
    Serial.print(atm_fx_compact_state.ticks);
    Serial.print(F(" serviced="));
    Serial.print(atm_fx_compact_state.serviced);
    Serial.print(F(" underruns="));
    Serial.print(atm_fx_compact_state.underruns);
    Serial.print(F(" error="));
    Serial.print(atm_fx_compact_state.error);
    Serial.print(F(" max_service_gap="));
    Serial.println(atm_fx_compact_state.max_service_gap);

    test.expectEq(atm_fx_compact_state.active, 1, F("audio player stays armed"));
    test.expectEq(atm_fx_compact_state.underruns, 0, F("audio loop has no underruns"));
    test.expectEq(atm_fx_compact_state.error, 0, F("audio stream format accepted"));
    test.expectEq(atm_fx_compact_state.max_service_gap <= 10, true,
                  F("audio service gap stays sane"));

    // Blank save sectors: partySlots are 255, so capture needs no store setup.
    gameState.state = GameState_t::WORLD;
    SaveController::begin(GameState_t::WORLD);
    test.expectEq(audio::isPaused(), true, F("save begin pauses audio"));
    for (uint16_t i = 0; i < 256 && gameState.state == GameState_t::SAVING; ++i) {
        SaveController::advance();
        FX::waitWhileBusy();
    }
    test.expectEq(gameState.state == GameState_t::SAVING, false,
                  F("save sequence completes"));
    test.expectEq(audio::isPaused(), false, F("save completion resumes audio"));

    // Routing smoke test (not a battle): state transitions select the intended
    // themes, and a bounded result cue falls back to the world theme on its own.
    enterBattle();
    test.expectEq(audio::currentTrack(), MusicTrack_Combat, F("battle entry selects combat"));
    exitBattle();
    test.expectEq(audio::currentTrack(), MusicTrack_Overworld,
                  F("battle exit selects overworld"));
    audio::playResult(MusicTrack_Victory, 2);
    test.expectEq(audio::currentTrack(), MusicTrack_Victory, F("result cue starts"));
    audio::service();
    delay(20);
    audio::service();
    test.expectEq(audio::currentTrack(), MusicTrack_Overworld,
                  F("result cue returns to overworld"));

    audio::stop();
}
#else
inline void run(FxTest &) {}
#endif
} // namespace readcounter_audio_detail

inline void test_readcounter(FxTest &test) {
    uint8_t row[16];
    FxReadCounter::resetFrame();
    test.expectEq(FxReadCounter::markUpdate(), true, F("idle update has zero reads"));
    test.expectEq(FxReadCounter::count(), static_cast<uint8_t>(0), F("reset count"));

    for (uint8_t i = 0; i < 4; ++i) {
        const uint24_t rowAddress = raw_map_data + static_cast<uint24_t>(i) * 512;
        fx_read_data_bytes(rowAddress, row, sizeof(row));
    }
    test.expectEq(FxReadCounter::count(), static_cast<uint8_t>(4), F("four row blocks"));
    test.expectEq(FxReadCounter::renderExact(4), true, F("four row budget"));
    test.expectEq(FxReadCounter::renderExact(5), false, F("four is not five"));

    fx_read_data_bytes(raw_map_data + static_cast<uint24_t>(4) * 512, row, sizeof(row));
    test.expectEq(FxReadCounter::renderExact(4), false, F("extra row fails exact four"));
    test.expectEq(FxReadCounter::renderExact(5), true, F("five row budget"));

    FxReadCounter::resetFrame();
    FxRead::indexed24(CreatureNames::CreatureNames, 0);
    test.expectEq(FxReadCounter::markUpdate(), false, F("lookup fails update budget"));

    FxReadCounter::resetFrame();
    FxRead::indexed24(CreatureNames::CreatureNames, 0);
    test.expectEq(FxReadCounter::transitionExact(1), true, F("table transition exact"));
    test.expectEq(FxReadCounter::markUpdate(), true, F("transition leaves zero update"));

    FxReadCounter::resetFrame();
    readCreatureNameAddress(0);
    test.expectEq(FxReadCounter::count(), static_cast<uint8_t>(1),
                  F("read from another translation unit counted"));

    readcounter_audio_detail::run(test);
}
