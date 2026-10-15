#include "SwitchPolicy.hpp"

#if defined(BATTLE_SIMULATOR) && !defined(__AVR__)

#include <avr/pgmspace.h>
#include "../../src/engine/battle/Ai.hpp"
#include "../../src/engine/battle/Damage.hpp"
#include "../../src/engine/battle/MoveUses.hpp"
#include "../../tst/fxdatatest/generated/creature_data.hpp"

namespace battle_sim {
namespace {

constexpr uint8_t EMPTY_SLOT = 255;

uint8_t benchForOriginal(const battle::BattleState &state, uint8_t side,
                         uint8_t original)
{
    uint8_t bench = 0;
    for (uint8_t slot = 0; slot < state.partyCount[side]; ++slot) {
        if (slot == state.activeSlot[side]) continue;
        if (slot == original) return bench;
        ++bench;
    }
    return EMPTY_SLOT;
}

const battle::BenchSlot *findBench(const battle::BattleState &state,
                                   uint8_t side, uint8_t original)
{
    const uint8_t bench = benchForOriginal(state, side, original);
    return bench < PARTY_SIZE - 1 ? &state.bench[side][bench] : nullptr;
}

uint8_t bestLegalDamage(const battle::BattleState &state,
                        battle::Side attackerSide,
                        const battle::Combatant &target)
{
    const uint8_t attackerIndex = static_cast<uint8_t>(attackerSide);
    const battle::Combatant &attacker = state.active[attackerIndex];
    uint8_t best = 0;
    for (uint8_t slot = 0; slot < 4; ++slot) {
        if (attacker.moveIds[slot] == EMPTY_SLOT ||
            battle::remainingMoveUses(state, attackerSide, slot) == 0) continue;
        const uint8_t damage = battle::computeDamage(attacker, target, slot);
        if (damage > best) best = damage;
    }
    return best;
}

battle::Combatant candidateFor(const battle::BattleState &state,
                               const SwitchProfile &profile,
                               const battle::BenchSlot &bench,
                               uint8_t originalSlot)
{
    battle::Combatant candidate = state.active[0];
    candidate.types = DualType(static_cast<Type>(profile.type1),
                               static_cast<Type>(profile.type2));
    candidate.stats.defense = profile.defense;
    candidate.stats.spcDef = profile.specialDefense;
    candidate.hp = bench.hp;
    candidate.statMods.modifiers = state.partyModifiers[0][originalSlot];
    // Bench statuses are cleared by the production switch path.
    candidate.status.clearEffects();
    candidate.effectTurns = 0;
    return candidate;
}

MenuIntent attackIntent(const battle::BattleState &state)
{
    const battle::BattleAction action =
        battle::chooseAction(state, battle::Side::Player);
    if (action.kind == battle::ActionKind::Attack && action.index < 4 &&
        state.active[0].moveIds[action.index] != EMPTY_SLOT) {
        return {MenuIntentKind::SelectMove, action.index};
    }
    return {MenuIntentKind::Pass, 0};
}

} // namespace

SwitchDecision chooseSwitchTacticalIntent(const Scenario &scenario,
                                          const battle::BattleState &state,
                                          bool switchLocked)
{
    const MenuIntent fallback = attackIntent(state);
    if (switchLocked || state.over || state.partyCount[0] <= 1 ||
        state.partyCount[0] > PARTY_SIZE || state.activeSlot[0] >= state.partyCount[0] ||
        state.active[0].hp == 0 || state.active[1].hp == 0 ||
        state.partyCount[1] == 0 || state.partyCount[1] > PARTY_SIZE) {
        return {fallback, SwitchReason::None};
    }

    const battle::BattleAction preferred =
        battle::chooseAction(state, battle::Side::Player);
    if (preferred.kind == battle::ActionKind::Attack &&
        battle::computeDamage(state.active[0], state.active[1], preferred.index) >=
            state.active[1].hp) {
        return {fallback, SwitchReason::None};
    }

    const uint8_t currentThreat = bestLegalDamage(
        state, battle::Side::Opponent, state.active[0]);
    if (currentThreat == 0 ||
        static_cast<uint16_t>(currentThreat) * 2u < state.active[0].hp) {
        return {fallback, SwitchReason::None};
    }

    uint8_t selected = EMPTY_SLOT;
    uint8_t selectedThreat = 0;
    uint8_t selectedHp = 1;
    for (uint8_t original = 0; original < state.partyCount[0]; ++original) {
        if (original == state.activeSlot[0]) continue;
        const battle::BenchSlot *bench = findBench(state, 0, original);
        if (bench == nullptr || bench->hp == 0 || bench->id >= creatureFixtureCount)
            continue;

        const battle::Combatant candidate = candidateFor(
            state, scenario.switchProfiles[0][original], *bench, original);
        const uint8_t threat = bestLegalDamage(
            state, battle::Side::Opponent, candidate);
        if (threat >= bench->hp) continue;

        // Require at least a 25% drop in expected damage per current HP.
        const uint32_t candidateRisk = static_cast<uint32_t>(threat) *
            state.active[0].hp * 4u;
        const uint32_t currentRiskLimit = static_cast<uint32_t>(currentThreat) *
            bench->hp * 3u;
        if (candidateRisk > currentRiskLimit) continue;

        if (selected == EMPTY_SLOT ||
            static_cast<uint32_t>(threat) * selectedHp <
                static_cast<uint32_t>(selectedThreat) * bench->hp) {
            selected = original;
            selectedThreat = threat;
            selectedHp = bench->hp;
        }
        // Equal risk keeps the first original party slot visited.
    }

    if (selected == EMPTY_SLOT) return {fallback, SwitchReason::None};
    return {{MenuIntentKind::SelectParty, selected},
            SwitchReason::ImprovedSurvival};
}

} // namespace battle_sim

#endif
