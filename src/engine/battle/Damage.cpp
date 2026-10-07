#include "Damage.hpp"
#include "BattleState.hpp"
#include "../../lib/Type.hpp"
#include "../../lib/Move.hpp"

namespace battle {
namespace {

struct StageScale {
    uint8_t numerator;
    uint8_t denominator;
};

static constexpr StageScale stageTable[9] = {
    {2, 6}, // -4
    {2, 5}, // -3
    {2, 4}, // -2
    {2, 3}, // -1
    {2, 2}, //  0
    {3, 2}, // +1
    {4, 2}, // +2
    {5, 2}, // +3
    {6, 2}, // +4
};

int8_t clampStage(int8_t stage)
{
    if (stage < -4) return -4;
    if (stage > 4) return 4;
    return stage;
}

bool validTableType(Type type)
{
    return type == Type::NONE || static_cast<uint8_t>(type) < TypeCount;
}

Modifier typeStatusModifier(const Combatant &attacker,
                            const Combatant &defender)
{
    const Modifier attackerModifier =
        typeEffectPairModifier(attacker.status.effects[0],
                               attacker.status.effects[1], attacker.types);

    const Modifier defenderModifier =
        inverseModifier(typeEffectPairModifier(defender.status.effects[0],
                                               defender.status.effects[1],
                                               defender.types));

    return combineModifier(attackerModifier, defenderModifier);
}

uint16_t applyDamageModifier(uint16_t value, Modifier modifier)
{
    // Caller has already rejected immunity; remaining modifiers encode shifts -2..2.
    const int8_t shift = static_cast<int8_t>(modifier) -
                         static_cast<int8_t>(Modifier::Same);
    return shift < 0 ? value >> -shift : value << shift;
}

} // namespace

uint16_t applyStage(uint16_t value, int8_t stage)
{
    const int8_t bounded = clampStage(stage);
    const StageScale &scale = stageTable[static_cast<uint8_t>(bounded + 4)];
    const uint32_t scaled =
        (static_cast<uint32_t>(value) * scale.numerator) / scale.denominator;
    return scaled > 0xffffU ? 0xffffU : static_cast<uint16_t>(scaled);
}

uint8_t computeDamage(const Combatant &attacker, const Combatant &defender,
                      uint8_t moveSlot)
{
    if (moveSlot >= 4) {
        return 0;
    }

    const Move &move = attacker.moves[moveSlot];
    const uint8_t moveTypeValue = move.getMoveType();
    const Type moveType = static_cast<Type>(moveTypeValue);
    if (!validTableType(moveType)) {
        return 0;
    }

    const uint8_t power = move.getMovePower();
    if (power == 0) {
        return 0;
    }

    const bool physical = move.isPhysical();
    const uint8_t attackValue = physical ? attacker.stats.attack
                                         : attacker.stats.spcAtk;
    const uint8_t defenseValue = physical ? defender.stats.defense
                                           : defender.stats.spcDef;
    const StatType attackStat = physical ? StatType::ATTACK_M
                                         : StatType::SPECIAL_ATTACK_M;
    const StatType defenseStat = physical ? StatType::DEFENSE_M
                                          : StatType::SPECIAL_DEFENSE_M;

    // Preserve the legacy power*attack and defense/2 terms, but stage each
    // term with bounded integer arithmetic before the division.
    const uint16_t attackBase =
        static_cast<uint16_t>(power) * static_cast<uint16_t>(attackValue);
    const uint16_t attackTerm =
        applyStage(attackBase, attacker.statMods.getModifier(attackStat));
    const uint16_t defenseBase = static_cast<uint16_t>(defenseValue / 2);
    const uint16_t defenseTerm =
        applyStage(defenseBase, defender.statMods.getModifier(defenseStat));
    const uint16_t divisor = defenseTerm == 0 ? 1 : defenseTerm;
    // Trial balance scale: give utility a turn before strong attacks end fights.
    // Packed power <=31, attack <=255, stage scale <=3, modifier <=4:
    // floor(31*255*3/2)*4 <=47428, so every damage intermediate fits uint16_t.
    const uint16_t baseDamage = attackTerm / divisor / 2;

    const Type defenderType1 = defender.types.getType1();
    const Type defenderType2 = defender.types.getType2();
    if (!validTableType(defenderType1) || !validTableType(defenderType2)) {
        return 0;
    }

    Modifier modifier = getModifier(moveType, defender.types);
    modifier = combineModifier(modifier, typeStatusModifier(attacker, defender));
    if (attacker.types.hasType(moveType)) {
        modifier = combineModifier(modifier, Modifier::Double);
    }
    if (modifier == Modifier::None) {
        return 0;
    }

    const uint16_t modifiedDamage = applyDamageModifier(baseDamage, modifier);
    if (modifiedDamage == 0) {
        return 1;
    }
    return modifiedDamage > 255U ? 255 : static_cast<uint8_t>(modifiedDamage);
}

} // namespace battle
