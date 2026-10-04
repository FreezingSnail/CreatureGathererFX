#pragma once

#include "BattleSession.hpp"
#include "BattleView.hpp"

namespace battle {

// Compatibility name for callers that only need a transient session view.
BattleView legacyBattleView(const BattleSession &session);

} // namespace battle
