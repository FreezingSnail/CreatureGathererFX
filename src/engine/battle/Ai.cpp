#include "Ai.hpp"

#include "Damage.hpp"

namespace battle {
namespace {

constexpr uint8_t SIDE_COUNT = 2;
constexpr uint8_t MOVE_COUNT = 4;
constexpr uint8_t EMPTY_MOVE = 255;

bool validSide(Side side)
{
    return static_cast<uint8_t>(side) < SIDE_COUNT;
}

bool validCombatant(const BattleState &state, Side side)
{
    const uint8_t index = static_cast<uint8_t>(side);
    return state.partyCount[index] != 0 &&
           state.partyCount[index] <= PARTY_SIZE &&
           state.activeSlot[index] < state.partyCount[index] &&
           state.active[index].hp != 0;
}

Side otherSide(Side side)
{
    return side == Side::Player ? Side::Opponent : Side::Player;
}

} // namespace

BattleAction chooseAction(const BattleState &state, Side side)
{
    if (!validSide(side) || state.over || !validCombatant(state, side)) {
        return {ActionKind::Skip, EMPTY_MOVE};
    }

    const Side targetSide = otherSide(side);
    if (!validCombatant(state, targetSide)) {
        return {ActionKind::Skip, EMPTY_MOVE};
    }

    const uint8_t actorIndex = static_cast<uint8_t>(side);
    const uint8_t targetIndex = static_cast<uint8_t>(targetSide);
    const Combatant &attacker = state.active[actorIndex];
    const Combatant &defender = state.active[targetIndex];

    uint8_t selected = EMPTY_MOVE;
    uint8_t bestDamage = 0;
    uint8_t bestPower = 0;
    for (uint8_t slot = 0; slot < MOVE_COUNT; ++slot) {
        // Semantic ID 255 is the only absent-move sentinel. ID zero remains a
        // legal move even when its packed move data has zero power.
        if (attacker.moveIds[slot] == EMPTY_MOVE) continue;

        const uint8_t damage = computeDamage(attacker, defender, slot);
        const uint8_t power = attacker.moves[slot].getMovePower();
        const bool firstLegal = selected == EMPTY_MOVE;
        const bool strongerDamage = damage > bestDamage;
        const bool zeroDamageFallback = damage == 0 && bestDamage == 0 &&
                                        power > bestPower;
        if (firstLegal || strongerDamage || zeroDamageFallback) {
            selected = slot;
            bestDamage = damage;
            bestPower = power;
        }
    }

    if (selected == EMPTY_MOVE) {
        return {ActionKind::Skip, EMPTY_MOVE};
    }
    return {ActionKind::Attack, selected};
}

} // namespace battle
