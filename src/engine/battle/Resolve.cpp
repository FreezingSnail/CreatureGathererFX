#include "Resolve.hpp"

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
    for (uint8_t side = 0; side < SIDE_COUNT; ++side) {
        out.speciesBefore[side] = state.active[side].id;
        out.maxHpBefore[side] = state.active[side].maxHp;
        out.hpBefore[side] = state.active[side].hp;
        out.hpAfter[side] = state.active[side].hp;
    }
    out.progressBefore = state.gather.progress;
    out.progressAfter = state.gather.progress;
}

void captureAfter(const BattleState &state, ActionResult &out)
{
    for (uint8_t side = 0; side < SIDE_COUNT; ++side) {
        out.hpAfter[side] = state.active[side].hp;
    }
    out.progressAfter = state.gather.progress;
}

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

uint32_t stagedSpeed(const Combatant &combatant)
{
    const int8_t stage = boundedSpeedStage(
        combatant.statMods.getModifier(StatType::SPEED_M));
    const uint32_t speed = combatant.stats.speed;
    if (stage < 0) {
        return (speed * 2u) / static_cast<uint32_t>(2 - stage);
    }
    return (speed * static_cast<uint32_t>(2 + stage)) / 2u;
}

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
    Modifier modifier = getModifier(moveType, defender.types);

    const Modifier attackerFirst =
        typeEffectModifier(attacker.status.effects[0], attacker.types);
    const Modifier attackerSecond =
        typeEffectModifier(attacker.status.effects[1], attacker.types);
    modifier = combineModifier(modifier,
                               combineModifier(attackerFirst, attackerSecond));

    const Modifier defenderFirst =
        typeEffectModifier(defender.status.effects[0], defender.types);
    const Modifier defenderSecond =
        typeEffectModifier(defender.status.effects[1], defender.types);
    modifier = combineModifier(
        modifier,
        inverseModifier(combineModifier(defenderFirst, defenderSecond)));

    if (attacker.types.hasType(moveType)) {
        modifier = combineModifier(modifier, Modifier::Double);
    }
    return modifier;
}

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

void resolveAttack(BattleState &state, Side actor, uint8_t moveSlot,
                   bool selfHit, Rng &rng, ActionResult &out)
{
    const uint8_t actorIndex = sideIndex(actor);
    const Side targetSide = selfHit ? actor : otherSide(actor);
    const uint8_t targetIndex = sideIndex(targetSide);
    Combatant &attacker = state.active[actorIndex];
    Combatant &target = state.active[targetIndex];

    out.kind = ResultKind::Attack;
    out.actor = actor;
    out.index = attacker.moveIds[moveSlot];
    out.effectiveness = attackEffectiveness(attacker, target, moveSlot);
    if (selfHit) out.flags |= SELF_HIT;

    if (target.hp != 0) {
        const uint8_t damage = computeDamage(attacker, target, moveSlot);
        target.hp = damage >= target.hp ? 0
            : static_cast<uint8_t>(target.hp - damage);
    }

    captureAfter(state, out);
    if (selfHit) {
        return;
    }

    // Effects resolve in authored slot order, after damage, and only while
    // their explicit target remains live. A faint target cannot receive a
    // status from the same move that defeated it.
    const Move &move = attacker.moves[moveSlot];
    const Effect effects[2] = {move.effect1, move.effect2};
    for (uint8_t slot = 0; slot < 2; ++slot) {
        const Effect effect = effects[slot];
        if (effect == Effect::NONE) continue;
        const Side effectTarget = isSelfEffect(effect) ? actor : targetSide;
        if (state.active[sideIndex(effectTarget)].hp == 0) continue;
        Consequence fact = {Effect::NONE, 255, 0};
        if (rollMoveEffect(state, actor, effect, rng, fact)) {
            appendFact(out, fact);
        }
    }
    captureAfter(state, out);
}

void resolveSkip(BattleState &state, Side actor, bool refused,
                 bool statusSkipped, ActionResult &out)
{
    (void)state;
    out.kind = ResultKind::Skip;
    out.actor = actor;
    if (refused) out.flags |= REFUSED;
    if (statusSkipped) out.flags |= STATUS_SKIPPED;
}

ResultKind extensionResult(ActionKind kind)
{
    switch (kind) {
    case ActionKind::Switch: return ResultKind::Switch;
    case ActionKind::Gather: return ResultKind::Gather;
    case ActionKind::Escape: return ResultKind::Escape;
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

Side firstMover(const BattleState &state, const TurnPlan &plan)
{
    const uint8_t playerPriority = actionPriority(plan.action[0].kind);
    const uint8_t opponentPriority = actionPriority(plan.action[1].kind);
    if (playerPriority != opponentPriority) {
        return playerPriority > opponentPriority ? Side::Player : Side::Opponent;
    }

    const uint32_t playerSpeed = stagedSpeed(state.active[sideIndex(Side::Player)]);
    const uint32_t opponentSpeed =
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
        const TurnGate gate = gateTurn(state, actor, rng);
        if (gate == TurnGate::Skip) {
            resolveSkip(state, actor, false, true, out);
            break;
        }
        resolveAttack(state, actor, action.index,
                      gate == TurnGate::SelfHit, rng, out);
        break;
    }
    case ActionKind::Skip:
        resolveSkip(state, actor, false, false, out);
        break;
    case ActionKind::Switch:
    case ActionKind::Gather:
    case ActionKind::Escape:
        // These transitions are owned by setup/.7, .8 and .9 respectively.
        // Keep a visible refused result here rather than importing their seams.
        out.kind = extensionResult(action.kind);
        out.actor = actor;
        out.flags |= REFUSED;
        break;
    }

    captureAfter(state, out);
    markFaints(out, out.flags);
    if (out.kind == ResultKind::Attack || out.kind == ResultKind::Skip) {
        setTerminalOutcome(state, out);
    }
}

void resolveEndTurn(BattleState &state, Rng &rng, bool acquisitionPending,
                    ActionResult &out)
{
    (void)rng;
    (void)acquisitionPending;
    resetActionResult(out);
    captureBefore(state, out);
    out.kind = ResultKind::EndTurn;
    if (state.over) {
        captureAfter(state, out);
        return;
    }

    tickEffects(state, out);
    captureAfter(state, out);
    markFaints(out, out.flags);
    setTerminalOutcome(state, out);
}

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
