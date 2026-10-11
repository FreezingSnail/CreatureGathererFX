#include "Ai.hpp"

#include "Damage.hpp"
#include "MoveUses.hpp"

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

BattleAction chooseDamageAction(const BattleState &state, Side side)
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
        if (attacker.moveIds[slot] == EMPTY_MOVE ||
            remainingMoveUses(state, side, slot) == 0) continue;

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

static BattleAction chooseNonSwitchAction(const BattleState &state, Side side)
{
    const BattleAction attack = chooseDamageAction(state, side);
    if (attack.kind != ActionKind::Attack) return attack;
    const uint8_t s = static_cast<uint8_t>(side);
    const Combatant &self = state.active[s];
    const Combatant &enemy = state.active[1 - s];
    const uint8_t damage = computeDamage(self, enemy, attack.index);
    if (damage >= enemy.hp) return attack;

    for (uint8_t slot = 0; slot < 4; ++slot) {
        if (remainingMoveUses(state, side, slot) == 0 ||
            self.moves[slot].getMovePower() != 0) continue;
        const Effect effect = self.moves[slot].effect1;
        if (effect == Effect::INFSED) {
            if (self.hp <= self.maxHp / 2 &&
                (self.status.effects[0] == Effect::NONE ||
                 self.status.effects[1] == Effect::NONE) &&
                self.status.effects[0] != effect && self.status.effects[1] != effect)
                return {ActionKind::Attack, slot};
            continue;
        }
        if (damage == 0 || self.hp <= self.maxHp / 2) continue;
        bool useful = false;
        switch (effect) {
        case Effect::ATKUP:
            useful = self.moves[attack.index].isPhysical() &&
                     self.statMods.getModifier(StatType::ATTACK_M) <= 0;
            break;
        case Effect::SPCAUP:
            useful = !self.moves[attack.index].isPhysical() &&
                     self.statMods.getModifier(StatType::SPECIAL_ATTACK_M) <= 0;
            break;
        case Effect::DEFUP: {
            const BattleAction enemyAttack = chooseDamageAction(state, otherSide(side));
            useful = enemyAttack.kind == ActionKind::Attack &&
                     enemy.moves[enemyAttack.index].isPhysical() &&
                     self.statMods.getModifier(StatType::DEFENSE_M) <= 0;
            break;
        }
        case Effect::SPDDWN:
            useful = enemy.statMods.getModifier(StatType::SPEED_M) >= 0 &&
                     applyStage(enemy.stats.speed, enemy.statMods.getModifier(StatType::SPEED_M)) >=
                     applyStage(self.stats.speed, self.statMods.getModifier(StatType::SPEED_M));
            break;
        default:
            break;
        }
        if (useful) return {ActionKind::Attack, slot};
    }
    return attack;
}

BattleAction chooseAction(const BattleState &state, Side side)
{
    return chooseNonSwitchAction(state, side);
}

} // namespace battle
