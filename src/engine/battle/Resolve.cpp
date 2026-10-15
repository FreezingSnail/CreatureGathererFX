#include "Resolve.hpp"
#include "MoveUses.hpp"

#include "Damage.hpp"
#include "Effects.hpp"
#include "../../lib/Type.hpp"

namespace battle {
namespace {

constexpr uint8_t SIDE_COUNT = 2;
constexpr uint8_t MOVE_COUNT = 4;
constexpr uint8_t PARTY_LIMIT = 3;

bool validSide(Side side)
{
    return static_cast<uint8_t>(side) < SIDE_COUNT;
}

Side otherSide(Side side)
{
    return side == Side::Player ? Side::Opponent : Side::Player;
}

uint8_t sideIndex(Side side)
{
    return static_cast<uint8_t>(side);
}

void captureBefore(const BattleState &state, ActionResult &out)
{
#ifdef CGFX_BATTLE_COMPACT_RESULT
    out.hpBefore[0] = state.active[0].hp;
    out.hpBefore[1] = state.active[1].hp;
#else
    for (uint8_t side = 0; side < SIDE_COUNT; ++side) {
        out.speciesBefore[side] = state.active[side].id;
        out.maxHpBefore[side] = state.active[side].maxHp;
        out.hpBefore[side] = state.active[side].hp;
        out.hpAfter[side] = state.active[side].hp;
    }
    out.progressBefore = state.gather.progress;
    out.progressAfter = state.gather.progress;
#endif
}

void captureAfter(const BattleState &state, ActionResult &out)
{
#ifndef CGFX_BATTLE_COMPACT_RESULT
    for (uint8_t side = 0; side < SIDE_COUNT; ++side) {
        out.hpAfter[side] = state.active[side].hp;
    }
    out.progressAfter = state.gather.progress;
#else
    (void)state;
    (void)out;
#endif
}

#ifdef CGFX_BATTLE_COMPACT_RESULT
void markFaints(const BattleState &state, const ActionResult &out, uint8_t &flags)
{
    for (uint8_t side = 0; side < SIDE_COUNT; ++side) {
        if (out.hpBefore[side] != 0 && state.active[side].hp == 0)
            flags |= side == 0 ? PLAYER_FAINTED : OPPONENT_FAINTED;
    }
}
#else
bool liveToZero(const ActionResult &out, Side side)
{
    const uint8_t index = sideIndex(side);
    return out.hpBefore[index] != 0 && out.hpAfter[index] == 0;
}

void markFaints(const ActionResult &out, uint8_t &flags)
{
    if (liveToZero(out, Side::Player)) flags |= PLAYER_FAINTED;
    if (liveToZero(out, Side::Opponent)) flags |= OPPONENT_FAINTED;
}
#endif

void setTerminalOutcome(BattleState &state, ActionResult &out)
{
    const bool playerDown = sideDefeated(state, Side::Player);
    const bool opponentDown = sideDefeated(state, Side::Opponent);
    if (playerDown) {
        out.outcome = Outcome::Lose;
        state.over = true;
    } else if (opponentDown) {
        out.outcome = Outcome::Win;
        state.over = true;
    }
}

int8_t boundedSpeedStage(int8_t stage)
{
    if (stage < -3) return -3;
    if (stage > 3) return 3;
    return stage;
}

#ifndef CGFX_DUEL_BATTLE
uint16_t stagedSpeed(const Combatant &combatant)
{
    const int8_t stage = boundedSpeedStage(
        combatant.statMods.getModifier(StatType::SPEED_M));
    const uint16_t speed = combatant.stats.speed;
    if (stage < 0) {
        return static_cast<uint16_t>((speed * 2u) / (2 - stage));
    }
    return static_cast<uint16_t>((speed * (2 + stage)) / 2u);
}
#endif

uint8_t actionPriority(ActionKind kind)
{
    switch (kind) {
    case ActionKind::Switch:
    case ActionKind::Gather:
    case ActionKind::Escape:
        return 1;
    case ActionKind::Attack:
    case ActionKind::Skip:
    default:
        return 0;
    }
}

bool validMove(const Combatant &combatant, uint8_t slot)
{
    return slot < MOVE_COUNT && combatant.moveIds[slot] != 255;
}

#ifndef CGFX_DUEL_BATTLE
Modifier attackEffectiveness(const Combatant &attacker,
                             const Combatant &defender, uint8_t moveSlot)
{
    if (!validMove(attacker, moveSlot)) return Modifier::Same;
    const uint8_t moveTypeValue = attacker.moves[moveSlot].getMoveType();
    const Type moveType = static_cast<Type>(moveTypeValue);
    if (moveType != Type::NONE && moveTypeValue >= TypeCount) {
        return Modifier::Same;
    }
    const Type defenderType1 = defender.types.getType1();
    const Type defenderType2 = defender.types.getType2();
    if ((defenderType1 != Type::NONE &&
         static_cast<uint8_t>(defenderType1) >= TypeCount) ||
        (defenderType2 != Type::NONE &&
         static_cast<uint8_t>(defenderType2) >= TypeCount)) {
        return Modifier::Same;
    }
    return attackModifier(attacker, defender, moveType);
}
#endif

#ifndef CGFX_BATTLE_NO_FACTS
bool appendFact(ActionResult &out, const Consequence &fact)
{
    if (fact.effect == Effect::NONE || fact.side >= SIDE_COUNT) return false;
    for (uint8_t slot = 0; slot < 4; ++slot) {
        if (out.consequences[slot].effect == Effect::NONE) {
            out.consequences[slot] = fact;
            return true;
        }
    }
    return false;
}
#endif

#ifndef CGFX_DUEL_BATTLE
void resolveAttack(BattleState &state, Side actor, uint8_t moveSlot,
                   bool selfHit, uint8_t remainingUses, Rng &rng,
                   ActionResult &out)
{
    if (!selfHit) spendMoveUse(state, actor, moveSlot, remainingUses);
    const uint8_t actorIndex = sideIndex(actor);
    const Side targetSide = selfHit ? actor : otherSide(actor);
    const uint8_t targetIndex = sideIndex(targetSide);
    Combatant &attacker = state.active[actorIndex];
    Combatant &target = state.active[targetIndex];
    const Move &move = attacker.moves[moveSlot];

    out.kind = ResultKind::Attack;
    out.actor = actor;
    out.index = attacker.moveIds[moveSlot];
#ifndef CGFX_BATTLE_COMPACT_RESULT
    out.effectiveness = attackEffectiveness(attacker, target, moveSlot);
#endif
    if (selfHit) out.flags |= SELF_HIT;
#ifndef CGFX_BATTLE_COMPACT_RESULT
    if (move.isPhysical()) out.flags |= PHYSICAL_MOVE;
#endif

    if (target.hp != 0) {
        const uint8_t damage = computeDamage(attacker, target, moveSlot);
        target.hp = damage >= target.hp ? 0
            : static_cast<uint8_t>(target.hp - damage);
    }

    if (selfHit) {
        return;
    }

    // Effects resolve in authored slot order, after damage, and only while
    // their explicit target remains live. A faint target cannot receive a
    // status from the same move that defeated it.
    const Effect effects[2] = {move.effect1, move.effect2};
    for (uint8_t slot = 0; slot < 2; ++slot) {
        const Effect effect = effects[slot];
        if (effect == Effect::NONE) continue;
        const Side effectTarget = isSelfEffect(effect) ? actor : targetSide;
        if (state.active[sideIndex(effectTarget)].hp == 0) continue;
#ifdef CGFX_BATTLE_NO_FACTS
        (void)rollMoveEffect(state, actor, effect, rng);
#else
        Consequence fact = {Effect::NONE, 255, 0};
        if (rollMoveEffect(state, actor, effect, rng, fact)) {
            appendFact(out, fact);
        }
#endif
    }
}
#endif

void resolveSkip(BattleState &state, Side actor, bool refused,
                 bool statusSkipped, ActionResult &out)
{
    (void)state;
    out.kind = ResultKind::Skip;
    out.actor = actor;
    if (refused) out.flags |= REFUSED;
    if (statusSkipped) out.flags |= STATUS_SKIPPED;
}

#ifndef CGFX_DUEL_BATTLE
void resolveGather(BattleState &state, Side actor, Rng &rng,
                   ActionResult &out)
{
    out.kind = ResultKind::Gather;
    out.actor = actor;

    // Gather is a wild encounter action. Availability/refusal is checked
    // before status gates so refused actions consume no RNG or progress.
    if (actor != Side::Player || state.trainer || !state.gatherable) {
        out.flags |= REFUSED;
        return;
    }

    const TurnGate gate = gateTurn(state, actor, rng);
    if (gate == TurnGate::Skip) {
        resolveSkip(state, actor, false, true, out);
        return;
    }
    if (gate == TurnGate::SelfHit) {
        if (validMove(state.active[sideIndex(actor)], 0)) {
            resolveAttack(state, actor, 0, true, 0, rng, out);
        } else {
            resolveSkip(state, actor, true, false, out);
        }
        return;
    }

    const uint8_t need = state.gather.need;
    if (state.gather.progress < need) {
        const uint16_t updated = static_cast<uint16_t>(state.gather.progress) +
                                 state.gather.tierRate;
        state.gather.progress = updated >= need
            ? need
            : static_cast<uint8_t>(updated);
    }
}
#endif

ResultKind extensionResult(ActionKind kind)
{
    switch (kind) {
    case ActionKind::Switch: return ResultKind::Switch;
    case ActionKind::Gather: return ResultKind::Gather;
    case ActionKind::Escape: return ResultKind::Escape;
    // Keep the legacy enum value stable while the battle item route is absent.
    case ActionKind::UseItem: return ResultKind::UseItem;
    case ActionKind::Attack: return ResultKind::Attack;
    case ActionKind::Skip:
    default: return ResultKind::Skip;
    }
}

uint8_t benchIndexForOriginalSlot(const BattleState &state, Side side,
                                  uint8_t originalSlot)
{
    const uint8_t index = sideIndex(side);
    uint8_t bench = 0;
    for (uint8_t slot = 0; slot < state.partyCount[index] &&
         slot < PARTY_LIMIT; ++slot) {
        if (slot == state.activeSlot[index]) continue;
        if (slot == originalSlot) return bench;
        ++bench;
    }
    return PARTY_LIMIT;
}

} // namespace

#ifndef CGFX_DUEL_BATTLE
Side firstMover(const BattleState &state, const TurnPlan &plan)
{
    const uint8_t playerPriority = actionPriority(plan.action[0].kind);
    const uint8_t opponentPriority = actionPriority(plan.action[1].kind);
    if (playerPriority != opponentPriority) {
        return playerPriority > opponentPriority ? Side::Player : Side::Opponent;
    }

    const uint16_t playerSpeed = stagedSpeed(state.active[sideIndex(Side::Player)]);
    const uint16_t opponentSpeed =
        stagedSpeed(state.active[sideIndex(Side::Opponent)]);
    return playerSpeed >= opponentSpeed ? Side::Player : Side::Opponent;
}

void resolveAction(BattleState &state, Side actor, BattleAction action,
                   Rng &rng, ActionResult &out)
{
    resetActionResult(out);
    captureBefore(state, out);
    if (!validSide(actor) || state.over) {
        captureAfter(state, out);
        return;
    }

    const uint8_t actorIndex = sideIndex(actor);
    if (state.active[actorIndex].hp == 0) {
        resolveSkip(state, actor, true, false, out);
        captureAfter(state, out);
        // A pending dead actor cannot act, but entry/transition callers still
        // need the terminal result when its real party has no live slot.
        setTerminalOutcome(state, out);
        return;
    }

    switch (action.kind) {
    case ActionKind::Attack: {
        if (!validMove(state.active[actorIndex], action.index)) {
            resolveSkip(state, actor, true, false, out);
            break;
        }
        const uint8_t remainingUses =
            remainingMoveUses(state, actor, action.index);
        if (remainingUses == 0) {
            resolveSkip(state, actor, true, false, out);
            break;
        }
        const TurnGate gate = gateTurn(state, actor, rng);
        if (gate == TurnGate::Skip) {
            resolveSkip(state, actor, false, true, out);
            break;
        }
        resolveAttack(state, actor, action.index,
                      gate == TurnGate::SelfHit, remainingUses, rng, out);
        break;
    }
    case ActionKind::Skip:
        resolveSkip(state, actor, false, false, out);
        break;
    case ActionKind::Switch:
        // Switch transitions are setup-owned. Keep a visible refused result
        // rather than importing their FX seam into resolution.
        out.kind = extensionResult(action.kind);
        out.actor = actor;
        out.flags |= REFUSED;
        break;
    case ActionKind::Gather:
        resolveGather(state, actor, rng, out);
        break;
    case ActionKind::UseItem:
        // World inventory remains available outside battle. This legacy action
        // value is refused until a battle item interface is deliberately added.
        resolveSkip(state, actor, true, false, out);
        break;
    case ActionKind::Escape:
        out.kind = ResultKind::Escape;
        out.actor = actor;
        if (state.trainer) {
            // Trainer battles never escape; refusal still consumes this one
            // action, so the caller may resolve the other actor normally.
            out.flags |= REFUSED;
        } else {
            // Keep the active combatants intact for terminal presentation.
            // The session observes over and cancels the other action/end turn.
            state.over = true;
            out.outcome = Outcome::Escaped;
        }
        break;
    }

    captureAfter(state, out);
#ifdef CGFX_BATTLE_COMPACT_RESULT
    markFaints(state, out, out.flags);
#else
    markFaints(out, out.flags);
#endif
    if (out.kind == ResultKind::Attack || out.kind == ResultKind::Skip) {
        setTerminalOutcome(state, out);
    }
}

void resolveEndTurn(BattleState &state, Rng &rng, bool acquisitionPending,
                    ActionResult &out)
{
    (void)rng;
    resetActionResult(out);
    captureBefore(state, out);
    out.kind = ResultKind::EndTurn;
    if (state.over) {
        captureAfter(state, out);
        return;
    }

#ifdef CGFX_BATTLE_NO_FACTS
    tickEffects(state);
#else
    tickEffects(state, out);
#endif
    captureAfter(state, out);
#ifdef CGFX_BATTLE_COMPACT_RESULT
    markFaints(state, out, out.flags);
#else
    markFaints(out, out.flags);
#endif

    // End-turn terminal order is intentionally explicit: a KO wins over a
    // pending gather, which wins over the wild flee countdown.
    setTerminalOutcome(state, out);
    if (out.outcome != Outcome::None) return;

    // A fainted active awaits replacement; it is not an active encounter for
    // acquisition or countdown purposes until the session switches it.
    if (state.active[static_cast<uint8_t>(Side::Player)].hp == 0 ||
        state.active[static_cast<uint8_t>(Side::Opponent)].hp == 0) {
        return;
    }

    if (acquisitionPending) {
        state.over = true;
        out.outcome = Outcome::Gathered;
        return;
    }

    if (!state.trainer && state.gatherable) {
        if (state.gather.fleeTurns != 0) {
            --state.gather.fleeTurns;
        }
        if (state.gather.fleeTurns == 0) {
            state.over = true;
            out.outcome = Outcome::Fled;
        }
    }
}

#endif

bool sideDefeated(const BattleState &state, Side side)
{
    if (!validSide(side)) return true;
    const uint8_t index = sideIndex(side);
    const uint8_t count = state.partyCount[index];
    if (count == 0 || count > PARTY_LIMIT ||
        state.activeSlot[index] >= count) {
        return true;
    }
    if (state.active[index].hp != 0) return false;

    const uint8_t benchCount = static_cast<uint8_t>(count - 1);
    for (uint8_t slot = 0; slot < benchCount; ++slot) {
        if (state.bench[index][slot].hp != 0) return false;
    }
    return true;
}

bool canSwitch(const BattleState &state, Side side, uint8_t originalSlot)
{
    if (!validSide(side) || state.over) return false;
    const uint8_t index = sideIndex(side);
    const uint8_t count = state.partyCount[index];
    if (count < 2 || count > PARTY_LIMIT || originalSlot >= count ||
        state.activeSlot[index] >= count ||
        originalSlot == state.activeSlot[index]) {
        return false;
    }
    const uint8_t benchSlot = benchIndexForOriginalSlot(state, side, originalSlot);
    return benchSlot < static_cast<uint8_t>(count - 1) &&
           state.bench[index][benchSlot].hp != 0;
}

} // namespace battle
