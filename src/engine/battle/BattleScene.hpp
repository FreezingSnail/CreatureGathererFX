#pragma once

#include "BattleState.hpp"

namespace battle {
namespace BattleScene {

// Draw the two active creatures and their RAM-resident HP directly from state.
void draw(const BattleState &state);

} // namespace BattleScene
} // namespace battle
