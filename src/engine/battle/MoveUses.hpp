#pragma once

#include "BattleState.hpp"

namespace battle {

constexpr uint8_t UNLIMITED_MOVE_USES = 255;

inline uint8_t moveUseLimit(const Move &move)
{
    if (move.getMovePower() == 0 &&
        (isStatEffect(move.effect1) || isStatEffect(move.effect2))) return 3;
    if (move.getMovePower() >= 10 || move.effect1 != Effect::NONE ||
        move.effect2 != Effect::NONE) return 2;
    return UNLIMITED_MOVE_USES;
}

inline uint8_t remainingMoveUses(const BattleState &state, Side side,
                                 uint8_t slot)
{
    const uint8_t s = static_cast<uint8_t>(side);
    if (s >= 2 || slot >= 4 || state.activeSlot[s] >= PARTY_SIZE ||
        state.active[s].moveIds[slot] == 255) return 0;
    const uint8_t limit = moveUseLimit(state.active[s].moves[slot]);
    if (limit == UNLIMITED_MOVE_USES) return limit;
    const uint8_t spent = (state.moveUsesSpent[s][state.activeSlot[s]] >>
                           (slot * 2)) & 3;
    return spent >= limit ? 0 : static_cast<uint8_t>(limit - spent);
}

inline void spendMoveUse(BattleState &state, Side side, uint8_t slot)
{
    const uint8_t remaining = remainingMoveUses(state, side, slot);
    if (remaining == 0 || remaining == UNLIMITED_MOVE_USES) return;
    state.moveUsesSpent[static_cast<uint8_t>(side)]
                       [state.activeSlot[static_cast<uint8_t>(side)]] +=
        static_cast<uint8_t>(1u << (slot * 2));
}

inline void resetMoveUses(BattleState &state)
{
    for (uint8_t side = 0; side < 2; ++side)
        for (uint8_t slot = 0; slot < PARTY_SIZE; ++slot)
            state.moveUsesSpent[side][slot] = 0;
}

} // namespace battle
