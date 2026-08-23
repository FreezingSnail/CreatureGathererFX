#pragma once

#include "../engine/game/Gamestate.hpp"

// Sketch-facing save driver. Owns the compaction buffer and the save screen so
// the sketch only has to route its SAVING state here; callers never touch
// SaveFile, the journal, or the step machine directly.
namespace SaveController {

// Snapshots live RAM and enters the multi-frame save sequence. returnState is
// the state to restore once the sequence finishes.
void begin(GameState_t returnState);

// Advances the save one step per frame and restores the pre-save state when the
// sequence completes. Starts a WORLD-returning save if none is in progress.
void advance();

// Draws save progress or failure. This path is deliberately RAM-only: reading
// FX while the save flash is busy corrupts the read.
void drawStatus();

} // namespace SaveController
