#pragma once

#include "BattleView.hpp"

class BattleEngine;

namespace battle {

// Temporary bridge for legacy battle callers. BattleSession replaces this in .11.
BattleView legacyBattleView(const BattleEngine &engine);

} // namespace battle
