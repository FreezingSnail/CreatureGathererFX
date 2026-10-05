#pragma once

#include <stdint.h>
#include "ActionResult.hpp"

namespace BattleFlow {

// Route one frame's fresh button edges to the resident battle. Returns true
// only after terminal feedback has completed and the world mode is restored.
using TerminalCallback = void (*)(battle::Outcome);
bool update(uint8_t edgeButtons, TerminalCallback onFinished = nullptr);

} // namespace BattleFlow
