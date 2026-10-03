#pragma once

#include "ActionResult.hpp"
#include "BattleState.hpp"

namespace battle {

// BattleSetup owns transition-time creature data. It imports persistent player
// HP without refilling it, caches active species/moves, and keeps all FX reads
// out of resolution and presentation paths.
void beginWild(BattleState &state, uint8_t creatureId, uint8_t level,
               bool gatherable, uint8_t tierRate);
void beginTrainer(BattleState &state, uint8_t opponentId);

// Legacy setup seam: benchIndex follows the current ascending bench order.
void loadActive(BattleState &state, Side side, uint8_t benchIndex);

// Switch by original party slot and emit a separate transition result.
bool applySwitch(BattleState &state, Side side, uint8_t originalSlot,
                 bool forced, ActionResult &out);

} // namespace battle
