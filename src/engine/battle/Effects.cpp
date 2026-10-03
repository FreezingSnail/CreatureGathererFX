#include "Effects.hpp"

#ifdef __AVR__
#include <avr/pgmspace.h>
#endif

namespace battle {
namespace {

constexpr uint8_t EFFECT_COUNT = static_cast<uint8_t>(Effect::CONCUSED) + 1;

#ifdef __AVR__
const uint8_t effectRates[EFFECT_COUNT] PROGMEM = {
#else
const uint8_t effectRates[EFFECT_COUNT] = {
#endif
    100, 100, 100, 100, 100, 100, 100, 100,
    100, 100, 100, 100, 100, 100, 100, 100,
    100, 100, 100, 100, 100, 100, 100, 100,
    100, 100, 100, 100, 100, 100,
};

bool validSide(Side side)
{
    return static_cast<uint8_t>(side) < 2;
}

Side otherSide(Side side)
{
    return side == Side::Player ? Side::Opponent : Side::Player;
}

bool validEffect(Effect effect)
{
    const uint8_t id = static_cast<uint8_t>(effect);
    return id < EFFECT_COUNT;
}

bool statForEffect(Effect effect, StatType &stat, int8_t &delta)
{
    switch (effect) {
    case Effect::ATKDWN: stat = StatType::ATTACK_M; delta = -1; return true;
    case Effect::DEFDWN: stat = StatType::DEFENSE_M; delta = -1; return true;
    case Effect::SPCADWN: stat = StatType::SPECIAL_ATTACK_M; delta = -1; return true;
    case Effect::SPCDDWN: stat = StatType::SPECIAL_DEFENSE_M; delta = -1; return true;
    case Effect::SPDDWN: stat = StatType::SPEED_M; delta = -1; return true;
    case Effect::ATKUP: stat = StatType::ATTACK_M; delta = 1; return true;
    case Effect::DEFUP: stat = StatType::DEFENSE_M; delta = 1; return true;
    case Effect::SPCAUP: stat = StatType::SPECIAL_ATTACK_M; delta = 1; return true;
    case Effect::SPCDUP: stat = StatType::SPECIAL_DEFENSE_M; delta = 1; return true;
    case Effect::SPDUP: stat = StatType::SPEED_M; delta = 1; return true;
    default: return false;
    }
}

bool appendFact(ActionResult &out, Effect effect, Side side, uint8_t value)
{
    for (uint8_t i = 0; i < 4; ++i) {
        Consequence &fact = out.consequences[i];
        if (fact.effect == Effect::NONE) {
            fact.effect = effect;
            fact.side = static_cast<uint8_t>(side);
            fact.value = value;
            return true;
        }
    }
    return false;
}

uint8_t tickAmount(const Combatant &combatant, Effect effect)
{
    const uint8_t amount = static_cast<uint8_t>(combatant.maxHp >> 4);
    return effect == Effect::SAPPD && amount == 0 ? 1 : amount;
}

bool applyTick(Combatant &combatant, Effect effect, uint8_t &after)
{
    if (combatant.hp == 0) {
        return false;
    }

    const uint8_t amount = tickAmount(combatant, effect);
    const uint8_t before = combatant.hp;
    if (effect == Effect::SAPPD) {
        combatant.hp = before <= amount ? 0 : static_cast<uint8_t>(before - amount);
    } else if (effect == Effect::INFSED) {
        const uint16_t healed = static_cast<uint16_t>(before) + amount;
        combatant.hp = healed > combatant.maxHp
            ? combatant.maxHp
            : static_cast<uint8_t>(healed);
    } else {
        return false;
    }

    after = combatant.hp;
    return after != before;
}

bool rollMoveEffectAtRate(BattleState &state, Side attacker, Effect effect,
                          Rng &rng, Consequence &out, uint8_t rate)
{
    if (!validEffect(effect) || effect == Effect::NONE) {
        return false;
    }
    if (rate > 100) {
        rate = 100;
    }
    if (rng.roll(100) >= rate) {
        return false;
    }

    const Side target = isSelfEffect(effect) ? attacker : otherSide(attacker);
    return applyEffect(state, target, effect, out);
}

} // namespace

uint8_t effectRate(uint8_t effectId)
{
    if (effectId >= EFFECT_COUNT) {
        return 0;
    }
#ifdef __AVR__
    return pgm_read_byte(effectRates + effectId);
#else
    return effectRates[effectId];
#endif
}

bool applyEffect(BattleState &state, Side target, Effect effect, Consequence &out)
{
    if (!validSide(target) || !validEffect(effect) || effect == Effect::NONE) {
        return false;
    }

    Combatant &combatant = state.active[static_cast<uint8_t>(target)];
    StatType stat = StatType::NONE;
    int8_t delta = 0;
    if (statForEffect(effect, stat, delta)) {
        const int8_t before = combatant.statMods.getModifier(stat);
        combatant.statMods.incrementModifier(stat, delta);
        const int8_t after = combatant.statMods.getModifier(stat);
        if (before == after) {
            return false;
        }
        out.effect = effect;
        out.side = static_cast<uint8_t>(target);
        out.value = static_cast<uint8_t>(after + 3);
        return true;
    }

    if (!combatant.status.applyEffect(effect)) {
        return false;
    }
    out.effect = effect;
    out.side = static_cast<uint8_t>(target);
    out.value = 0;
    return true;
}

bool rollMoveEffect(BattleState &state, Side attacker, Effect effect,
                    Rng &rng, Consequence &out)
{
    return rollMoveEffectAtRate(state, attacker, effect, rng, out,
                                effectRate(static_cast<uint8_t>(effect)));
}

bool rollMoveEffect(BattleState &state, Side attacker, Effect effect,
                    Rng &rng, Consequence &out, EffectRateFn rateFn)
{
    if (!validEffect(effect) || effect == Effect::NONE) {
        return false;
    }
    const uint8_t rate = rateFn == nullptr
        ? effectRate(static_cast<uint8_t>(effect))
        : rateFn(static_cast<uint8_t>(effect));
    return rollMoveEffectAtRate(state, attacker, effect, rng, out, rate);
}

bool rollMoveEffect(BattleState &state, Side attacker, Effect effect,
                    Rng &rng, Consequence &out, uint8_t rate)
{
    return rollMoveEffectAtRate(state, attacker, effect, rng, out, rate);
}

void tickEffects(BattleState &state, ActionResult &out)
{
    for (uint8_t sideIndex = 0; sideIndex < 2; ++sideIndex) {
        Combatant &combatant = state.active[sideIndex];
        const Side side = static_cast<Side>(sideIndex);
        for (uint8_t slot = 0; slot < 2; ++slot) {
            const Effect effect = combatant.status.effects[slot];
            if (effect != Effect::SAPPD && effect != Effect::INFSED) {
                continue;
            }
            uint8_t after = 0;
            if (applyTick(combatant, effect, after)) {
                appendFact(out, effect, side, after);
            }
        }
    }
}

TurnGate gateTurn(BattleState &state, Side side, Rng &rng)
{
    if (!validSide(side)) {
        return TurnGate::None;
    }

    const Combatant &combatant = state.active[static_cast<uint8_t>(side)];
    for (uint8_t slot = 0; slot < 2; ++slot) {
        switch (combatant.status.effects[slot]) {
        case Effect::PINNED:
            if (rng.roll(3) == 0) {
                return TurnGate::Skip;
            }
            break;
        case Effect::CONCUSED:
            if (rng.roll(4) == 0) {
                return TurnGate::SelfHit;
            }
            break;
        default:
            break;
        }
    }
    return TurnGate::None;
}

} // namespace battle
