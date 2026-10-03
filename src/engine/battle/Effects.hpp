#pragma once

#include "ActionResult.hpp"
#include "BattleRng.hpp"
#include "BattleState.hpp"
#include "../../lib/Effect.hpp"

namespace battle {

// The first triggered status gate wins; the resolver maps SelfHit to its
// selected move and Skip to a status-skipped result.
enum class TurnGate : uint8_t {
    None = 0,
    Skip = 1,
    SelfHit = 2,
    NoEffect = None,
    SkipTurn = Skip,
    TargetSelf = SelfHit,
};

// Optional host seam for exercising non-100 effect rates without making rate
// data part of BattleState or the AVR RNG object.
using EffectRateFn = uint8_t (*)(uint8_t effectId);

uint8_t effectRate(uint8_t effectId);

bool applyEffect(BattleState &state, Side target, Effect effect, Consequence &out);

bool rollMoveEffect(BattleState &state, Side attacker, Effect effect,
                    Rng &rng, Consequence &out);
bool rollMoveEffect(BattleState &state, Side attacker, Effect effect,
                    Rng &rng, Consequence &out, EffectRateFn rateFn);
bool rollMoveEffect(BattleState &state, Side attacker, Effect effect,
                    Rng &rng, Consequence &out, uint8_t rate);

void tickEffects(BattleState &state, ActionResult &out);
TurnGate gateTurn(BattleState &state, Side side, Rng &rng);

} // namespace battle
