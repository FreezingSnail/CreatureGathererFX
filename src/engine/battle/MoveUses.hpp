#pragma once

#include "BattleState.hpp"

namespace battle {

// The duel reuses the per-member move-use byte as the setup-cached attack-type
// mask (bit i = duel type i), so setup only zeroes it before the masks load.
inline void resetMoveUses(BattleState &state)
{
    for (uint8_t side = 0; side < 2; ++side)
        for (uint8_t slot = 0; slot < PARTY_SIZE; ++slot)
            state.moveUsesSpent[side][slot] = 0;
}

} // namespace battle
