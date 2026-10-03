#pragma once

#include "test.hpp"
#include "../src/engine/battle/Effects.hpp"

namespace battle_effects_test_detail {

uint8_t scriptedValues[16];
uint8_t scriptedCount = 0;
uint8_t scriptedIndex = 0;

uint8_t scriptedRoll(uint8_t)
{
    ++scriptedCount;
    if (scriptedIndex >= scriptedCount) {
        return 0;
    }
    return scriptedValues[scriptedIndex++];
}

uint8_t rate50(uint8_t)
{
    return 50;
}

void script(uint8_t first, uint8_t second = 0, uint8_t third = 0, uint8_t fourth = 0)
{
    scriptedValues[0] = first;
    scriptedValues[1] = second;
    scriptedValues[2] = third;
    scriptedValues[3] = fourth;
    scriptedCount = 0;
    scriptedIndex = 0;
}

battle::BattleState stateFixture(uint8_t playerHp = 100, uint8_t opponentHp = 100)
{
    battle::BattleState state = {};
    state.active[0].hp = playerHp;
    state.active[0].maxHp = 100;
    state.active[0].stats.hp = 100;
    state.active[1].hp = opponentHp;
    state.active[1].maxHp = 100;
    state.active[1].stats.hp = 100;
    return state;
}

uint8_t factCount(const battle::ActionResult &result)
{
    uint8_t count = 0;
    while (count < 4 && result.consequences[count].effect != Effect::NONE) {
        ++count;
    }
    return count;
}

} // namespace battle_effects_test_detail

inline void BattleEffectsIntegrationTest(TestSuite &suite)
{
    using namespace battle;
    using namespace battle_effects_test_detail;
    Test test(__func__);

    test.assert(isSelfEffect(Effect::ENLTND), true, "positive type effect targets self");
    test.assert(isSelfEffect(Effect::EVOLVD), true, "last positive type effect targets self");
    test.assert(isSelfEffect(Effect::ATKUP), true, "positive stat effect targets self");
    test.assert(isSelfEffect(Effect::SPDUP), true, "last positive stat effect targets self");
    test.assert(isSelfEffect(Effect::INFSED), true, "healing tick targets self");
    test.assert(isSelfEffect(Effect::DPRSD), false, "negative type effect targets other");
    test.assert(isSelfEffect(Effect::ATKDWN), false, "negative stat effect targets other");
    test.assert(isSelfEffect(Effect::SAPPD), false, "damage tick targets other");
    test.assert(isSelfEffect(Effect::NONE), false, "NONE never targets self");
    test.assert(effectRate(static_cast<uint8_t>(Effect::ATKUP)), 100,
                "recognized effect rate defaults to 100");
    test.assert(effectRate(255), 0, "NONE has no effect rate");

    battle::BattleState state = stateFixture();
    Consequence fact = {Effect::NONE, 255, 0};
    test.assert(applyEffect(state, Side::Opponent, Effect::ATKDWN, fact), true,
                "stat effect applies to explicit target");
    test.assert(state.active[1].statMods.getModifier(StatType::ATTACK_M), -1,
                "stat effect changes opponent stage");
    test.assert(fact.side, static_cast<uint8_t>(Side::Opponent), "stat fact names target side");
    test.assert(fact.value, 2, "stat fact stores stage plus three");
    for (uint8_t i = 0; i < 2; ++i) {
        test.assert(applyEffect(state, Side::Opponent, Effect::ATKDWN, fact), true,
                    "stat stage reaches lower cap");
    }
    test.assert(applyEffect(state, Side::Opponent, Effect::ATKDWN, fact), false,
                "capped stat emits no successful change");
    test.assert(state.active[1].statMods.getModifier(StatType::ATTACK_M), -3,
                "negative stat stage clamps at minus three");

    test.assert(applyEffect(state, Side::Opponent, Effect::SOAKED, fact), true,
                "status effect occupies first slot");
    test.assert(applyEffect(state, Side::Opponent, Effect::BUFTD, fact), true,
                "status effect occupies second slot");
    test.assert(applyEffect(state, Side::Opponent, Effect::DPRSD, fact), false,
                "full status slots drop effect");
    test.assert(applyEffect(state, Side::Opponent, Effect::NONE, fact), false,
                "NONE is rejected by direct application");

    script(0);
    Rng rng = {scriptedRoll};
    state = stateFixture(80, 80);
    test.assert(rollMoveEffect(state, Side::Player, Effect::ATKUP, rng, fact), true,
                "positive move effect rolls successfully");
    test.assert(state.active[0].statMods.getModifier(StatType::ATTACK_M), 1,
                "positive move effect targets attacker");
    test.assert(state.active[1].statMods.getModifier(StatType::ATTACK_M), 0,
                "positive move effect leaves defender unchanged");
    test.assert(rollMoveEffect(state, Side::Player, Effect::ATKDWN, rng, fact), true,
                "negative move effect rolls successfully");
    test.assert(state.active[1].statMods.getModifier(StatType::ATTACK_M), -1,
                "negative move effect targets defender");
    test.assert(rollMoveEffect(state, Side::Player, Effect::SAPPD, rng, fact), true,
                "damage tick move effect rolls successfully");
    test.assert(state.active[1].status.effects[0], Effect::SAPPD,
                "damage tick targets defender status slots");
    test.assert(state.active[1].hp, 80, "damage tick effect waits for end turn");
    test.assert(rollMoveEffect(state, Side::Player, Effect::INFSED, rng, fact), true,
                "healing tick move effect rolls successfully");
    test.assert(state.active[0].status.effects[0], Effect::INFSED,
                "healing tick targets attacker status slots");
    test.assert(state.active[0].hp, 80, "healing tick effect waits for end turn");
    const uint8_t rollsBeforeNone = scriptedCount;
    test.assert(rollMoveEffect(state, Side::Player, Effect::NONE, rng, fact), false,
                "NONE does not roll or apply");
    test.assert(scriptedCount, rollsBeforeNone, "NONE consumes no injected roll");

    script(50);
    state = stateFixture();
    test.assert(rollMoveEffect(state, Side::Player, Effect::ATKUP, rng, fact, rate50), false,
                "injected rate rejects boundary roll");
    test.assert(state.active[0].statMods.getModifier(StatType::ATTACK_M), 0,
                "rejected rate leaves state unchanged");
    script(49);
    test.assert(rollMoveEffect(state, Side::Player, Effect::ATKUP, rng, fact, rate50), true,
                "injected rate accepts below-boundary roll");
    test.assert(state.active[0].statMods.getModifier(StatType::ATTACK_M), 1,
                "accepted rate applies exactly once");

    state = stateFixture();
    state.active[0].status.effects[0] = Effect::PINNED;
    state.active[0].status.effects[1] = Effect::CONCUSED;
    script(1, 0);
    test.assert(gateTurn(state, Side::Player, rng), TurnGate::SelfHit,
                "gate scans past failed pinned slot to concusion");
    script(0);
    test.assert(gateTurn(state, Side::Player, rng), TurnGate::Skip,
                "pinned one-in-three gate wins in slot order");
    state.active[0].status.effects[0] = Effect::NONE;
    state.active[0].status.effects[1] = Effect::NONE;
    test.assert(gateTurn(state, Side::Player, rng), TurnGate::None,
                "empty gate returns none");

    state = stateFixture();
    state.active[0].status.effects[0] = Effect::SAPPD;
    state.active[0].status.effects[1] = Effect::INFSED;
    state.active[1].status.effects[0] = Effect::SAPPD;
    state.active[1].status.effects[1] = Effect::INFSED;
    ActionResult result;
    resetActionResult(result);
    tickEffects(state, result);
    test.assert(factCount(result), 4, "tick facts use all four capacity slots");
    test.assert(result.consequences[0].effect, Effect::SAPPD, "player slot zero ticks first");
    test.assert(result.consequences[0].side, 0, "first tick names player");
    test.assert(result.consequences[0].value, 94, "first tick stores intermediate HP");
    test.assert(result.consequences[1].effect, Effect::INFSED, "player slot one ticks second");
    test.assert(result.consequences[1].value, 100, "heal stores capped intermediate HP");
    test.assert(result.consequences[2].side, 1, "opponent slots follow player slots");
    test.assert(result.consequences[2].value, 94, "opponent damage stores intermediate HP");
    test.assert(result.consequences[3].value, 100, "opponent heal stores final HP");
    test.assert(state.active[0].hp, 100, "mixed player ticks net to original HP");
    test.assert(state.active[1].hp, 100, "mixed opponent ticks net to original HP");

    state = stateFixture(8, 1);
    state.active[0].status.effects[0] = Effect::SAPPD;
    state.active[0].status.effects[1] = Effect::INFSED;
    state.active[1].status.effects[0] = Effect::SAPPD;
    state.active[1].status.effects[1] = Effect::INFSED;
    resetActionResult(result);
    tickEffects(state, result);
    test.assert(factCount(result), 3, "zero absorption and death suppress revival fact");
    test.assert(result.consequences[0].value, 2, "small HP damage floors tick to one");
    test.assert(result.consequences[1].value, 8, "heal observes intermediate damaged HP");
    test.assert(result.consequences[2].value, 0, "damage can absorb final HP to zero");
    test.assert(state.active[1].hp, 0, "zero HP remains absorbing against INFSED");

    state = stateFixture(0, 100);
    state.active[0].status.effects[0] = Effect::INFSED;
    resetActionResult(result);
    tickEffects(state, result);
    test.assert(factCount(result), 0, "INFSED never revives a zero-HP combatant");
    test.assert(state.active[0].hp, 0, "zero HP stays zero");

    state = stateFixture(8, 7);
    state.active[0].maxHp = state.active[0].stats.hp = 8;
    state.active[1].maxHp = state.active[1].stats.hp = 8;
    state.active[0].status.effects[0] = Effect::SAPPD;
    state.active[1].status.effects[0] = Effect::INFSED;
    resetActionResult(result);
    tickEffects(state, result);
    test.assert(factCount(result), 1, "small HP damage has a minimum while heal floors naturally");
    test.assert(result.consequences[0].value, 7, "small SAPPD applies one damage");
    test.assert(state.active[1].hp, 7, "small INFSED does not round zero heal up");

    suite.addTest(test);
}

inline void BattleEffectsSuite(TestRunner &runner)
{
    TestSuite suite("Battle effects integration");
    BattleEffectsIntegrationTest(suite);
    runner.addTestSuite(suite);
}
