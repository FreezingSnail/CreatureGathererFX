#pragma once

#include "ArenaTypes.hpp"

namespace arena {

// Applies one frame's already edge-qualified input to arena UI state. This
// function is intentionally independent of the game, player, and FX layers.
ArenaIntent navigate(ArenaUiState &ui, ArenaContext &context,
                     uint8_t edgeButtons, uint8_t playerCount,
                     uint8_t opponentCount);

} // namespace arena
