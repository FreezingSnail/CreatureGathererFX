#pragma once

#include "BattleState.hpp"

namespace battle {
namespace BattleScene {

// Draw the two active creatures and their RAM-resident HP directly from state.
// Under CGFX_DUEL_BATTLE this also draws the type badges, team pips and either
// the compact pick row + centre (duelCursor >= 0, no result) or the result
// feedback footer (state.feedback != 0). duelCursor is ignored otherwise.
void draw(const BattleState &state, int8_t duelCursor = -1);

} // namespace BattleScene
} // namespace battle
