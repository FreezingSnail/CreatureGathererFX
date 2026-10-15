#pragma once

#include "BattleState.hpp"

namespace battle {

// Choose a reproducible attack for an active combatant. The returned index is
// a move slot, not a semantic move ID; absent slots use move ID 255. Invalid,
// terminal, empty, or fainted states return Skip/255. This boundary is RAM-only
// and consumes no random input, FX data, or global state.
BattleAction chooseDamageAction(const BattleState &state, Side side);

// PP-aware tactical choice: lethal attacks, regeneration, then useful setup.
BattleAction chooseAction(const BattleState &state, Side side);

} // namespace battle
