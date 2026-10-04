#pragma once

#include <stdint.h>

namespace BattleFlow {

// Route one frame's fresh button edges to the resident battle. Returns true
// only after terminal feedback has completed and the world mode is restored.
bool update(uint8_t edgeButtons);

} // namespace BattleFlow
