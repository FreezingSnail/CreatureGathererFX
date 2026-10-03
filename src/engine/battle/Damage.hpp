#pragma once

#include <stdint.h>
#include "BattleState.hpp"

namespace battle {

// Pure battle math: no FX reads, globals, events, HP writes, or randomness.
uint8_t computeDamage(const Combatant &attacker, const Combatant &defender,
                      uint8_t moveSlot);
uint16_t applyStage(uint16_t value, int8_t stage);

} // namespace battle
