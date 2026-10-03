#pragma once

#include "ActionResult.hpp"
#include "BattleRng.hpp"
#include "BattleState.hpp"

namespace battle {

// The cursor owns the gather-ready bit; this flag remains outside BattleState.
enum : uint8_t {
    ACQUISITION_PENDING = 1u << 0
};

Side firstMover(const BattleState &state, const TurnPlan &plan);
void resolveAction(BattleState &state, Side actor, BattleAction action,
                   Rng &rng, ActionResult &out);
void resolveEndTurn(BattleState &state, Rng &rng, bool acquisitionPending,
                    ActionResult &out);

bool sideDefeated(const BattleState &state, Side side);
bool canSwitch(const BattleState &state, Side side, uint8_t originalSlot);

} // namespace battle
