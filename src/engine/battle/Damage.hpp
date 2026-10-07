#pragma once

#include <stdint.h>
#include "BattleState.hpp"

namespace battle {

// Pure battle math: no FX reads, globals, events, HP writes, or randomness.
uint8_t computeDamage(const Combatant &attacker, const Combatant &defender,
                      uint8_t moveSlot);
// Inputs must have passed the caller's move and defender type checks.
// Shares damage's modifier order for both HP damage and displayed effectiveness.
Modifier attackModifier(const Combatant &attacker, const Combatant &defender,
                        Type moveType);
uint16_t applyStage(uint16_t value, int8_t stage);

} // namespace battle
