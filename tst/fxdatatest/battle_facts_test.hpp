#pragma once

#include "fxtest.hpp"
#include "src/engine/battle/Resolve.hpp"

namespace battle_facts_test_detail {
static uint8_t draws;

inline uint8_t roll(uint8_t bound)
{
    ++draws;
    (void)bound;
    return 0;
}

inline battle::BattleState state()
{
    battle::BattleState result = {};
    result.trainer = true;
    for (uint8_t side = 0; side < 2; ++side) {
        battle::Combatant &creature = result.active[side];
        creature.id = side + 1;
        creature.types = DualType(Type::SPIRIT, Type::NONE);
        creature.hp = creature.maxHp = 100;
        creature.stats.hp = 100;
        creature.stats.attack = 40;
        creature.stats.defense = 20;
        creature.stats.spcAtk = 40;
        creature.stats.spcDef = 20;
        creature.stats.speed = 10;
        for (uint8_t slot = 0; slot < 4; ++slot) creature.moveIds[slot] = 255;
        creature.moveIds[0] = 7;
        creature.moves[0] = Move(MoveBitSet{static_cast<uint8_t>(Type::SPIRIT), 10, 1, 0, 0});
        result.partyCount[side] = 1;
    }
    return result;
}
}

inline void test_battle_facts(FxTest &test)
{
    using namespace battle;
    using namespace battle_facts_test_detail;
    Rng rng = {roll};
    ActionResult result = {};

    battle::BattleState stateUnderTest = state();
    stateUnderTest.active[0].hp = 40;
    stateUnderTest.active[0].moves[0].effect1 = Effect::ATKUP;
    stateUnderTest.active[0].moves[0].effect2 = Effect::SAPPD;
    draws = 0;
    resolveAction(stateUnderTest, Side::Player, {ActionKind::Attack, 0}, rng, result);
    test.expectEq(static_cast<uint8_t>(result.kind), static_cast<uint8_t>(ResultKind::Attack), F("effect move resolves"));
    test.expectEq(stateUnderTest.active[0].statMods.getModifier(StatType::ATTACK_M), 1,
                  F("self stage effect still applies"));
    test.expectEq(static_cast<uint8_t>(stateUnderTest.active[1].status.effects[0]), static_cast<uint8_t>(Effect::SAPPD),
                  F("opponent status effect still applies"));
    test.expectEq(stateUnderTest.active[1].hp, 60, F("attack damage still applies before effects"));
    test.expectEq(draws, 2, F("both successful effects keep their RNG draws"));

    stateUnderTest.active[0].hp = 40;
    stateUnderTest.active[0].status.effects[0] = Effect::INFSED;
    stateUnderTest.active[0].effectTurns = 3;
    draws = 0;
    resolveEndTurn(stateUnderTest, rng, false, result);
    test.expectEq(stateUnderTest.active[0].hp, 52, F("end-turn healing still applies"));
    test.expectEq(stateUnderTest.active[1].hp, 54, F("end-turn damage still applies"));
    test.expectEq((stateUnderTest.active[0].effectTurns & 3), 2, F("timed effect still decrements"));
    test.expectEq(result.hpAfter[0], 52, F("feedback HP snapshot remains available"));
    test.expectEq(result.hpAfter[1], 54, F("both sequential tick results remain visible"));
    test.expectEq(draws, 0, F("end-turn effects add no RNG draws"));

    stateUnderTest = state();
    stateUnderTest.active[1].hp = 1;
    stateUnderTest.active[0].moves[0].effect1 = Effect::ATKUP;
    stateUnderTest.active[0].moves[0].effect2 = Effect::SAPPD;
    draws = 0;
    resolveAction(stateUnderTest, Side::Player, {ActionKind::Attack, 0}, rng, result);
    test.expectEq(stateUnderTest.active[1].hp, 0, F("lethal damage still knocks out target"));
    test.expectEq((result.flags & OPPONENT_FAINTED) != 0, true,
                  F("KO feedback flag remains available"));
    test.expectEq(static_cast<uint8_t>(result.outcome), static_cast<uint8_t>(Outcome::Win), F("last target knockout still ends battle"));
    test.expectEq(static_cast<uint8_t>(stateUnderTest.active[1].status.effects[0]), static_cast<uint8_t>(Effect::NONE),
                  F("effect does not apply after target faints"));
    test.expectEq(draws, 1, F("KO skips the later effect RNG draw"));
}
