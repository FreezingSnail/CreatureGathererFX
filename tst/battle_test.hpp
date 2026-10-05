#pragma once
#include "test.hpp"
#include "../src/engine/battle/BattleState.hpp"
#include "../src/engine/battle/ActionResult.hpp"
#include "../src/engine/battle/Damage.hpp"

#include <stddef.h>
#include <type_traits>

static_assert(sizeof(Move) == 4, "packed move must remain four bytes");
static_assert(sizeof(StatusEffect) == 2, "two status slots must remain two bytes");
static_assert(sizeof(battle::BenchSlot) == 6, "bench caches defensive profile");
static_assert(sizeof(battle::BattleState) <= 128, "native state includes cached switch defense profiles");
static_assert(sizeof(battle::BattleAction) == 2, "action is kind and index");
static_assert(sizeof(battle::TurnPlan) == 4, "two actions only");
static_assert(sizeof(battle::GatherState) == 4, "gather facts are byte sized");
static_assert(sizeof(battle::ActionKind) == 1, "action kind is byte sized");
static_assert(sizeof(battle::ResultKind) == 1, "result kind is byte sized");
static_assert(sizeof(battle::Outcome) == 1, "outcome is byte sized");
static_assert(sizeof(battle::Consequence) == 3, "semantic fact is three bytes");
static_assert(sizeof(battle::ActionResult) == 28, "result capacity is fixed");
static_assert(offsetof(battle::ActionResult, speciesBefore) == 6, "before species offset");
static_assert(offsetof(battle::ActionResult, maxHpBefore) == 8, "before max HP offset");
static_assert(offsetof(battle::ActionResult, hpBefore) == 10, "before HP offset");
static_assert(offsetof(battle::ActionResult, hpAfter) == 12, "after HP offset");
static_assert(offsetof(battle::ActionResult, progressBefore) == 14, "before progress offset");
static_assert(offsetof(battle::ActionResult, progressAfter) == 15, "after progress offset");
static_assert(offsetof(battle::ActionResult, consequences) == 16, "semantic facts offset");
static_assert(std::is_trivially_copyable<battle::ActionResult>::value,
              "result has no ownership or mutable combatant pointers");
static_assert(std::is_trivial<battle::ActionResult>::value,
              "result initialization stays explicit and avoids SRAM templates");

void BattleDataContractTest(TestSuite &suite) {
    Test test(__func__);
    battle::ActionResult result;
    battle::resetActionResult(result);
    test.assert(result.kind, battle::ResultKind::None, "fresh output has no action");
    test.assert(result.index, static_cast<uint8_t>(255), "fresh output has no semantic ID");
    test.assert(result.flags, static_cast<uint8_t>(0), "fresh output has no flags");
    test.assert(result.outcome, battle::Outcome::None, "fresh output has no terminal outcome");
    test.assert(result.effectiveness, Modifier::Same, "non-attack effectiveness is neutral");
    for (const battle::Consequence &fact : result.consequences) {
        test.assert(fact.effect, Effect::NONE, "unused effect sentinel");
        test.assert(fact.side, static_cast<uint8_t>(255), "unused side cannot name an actor");
        test.assert(fact.value, static_cast<uint8_t>(0), "unused fact carries no value");
    }
    test.assert(static_cast<uint8_t>(battle::SELF_HIT | battle::REFUSED |
                battle::STATUS_SKIPPED | battle::PLAYER_FAINTED |
                battle::OPPONENT_FAINTED | battle::FORCED_SWITCH),
                static_cast<uint8_t>(0x3f), "top two flag bits stay reserved");

    battle::BattleState state = {};
    state.active[0].hp = 255;
    state.active[0].moveIds[0] = 0;
    state.active[0].moveIds[1] = 32;
    state.active[0].moveIds[2] = 255;
    test.assert(state.active[0].hp, static_cast<uint8_t>(255), "HP preserves the byte ceiling");
    test.assert(state.active[0].moveIds[0], static_cast<uint8_t>(0), "zero is a semantic move ID");
    test.assert(state.active[0].moveIds[1], static_cast<uint8_t>(32), "canonical empty move ID preserved");
    test.assert(state.active[0].moveIds[2], static_cast<uint8_t>(255), "absent move ID preserved");
    for (const battle::Combatant &combatant : state.active) {
        test.assert(combatant.status.effects[0], Effect::NONE, "fresh first status is absent");
        test.assert(combatant.status.effects[1], Effect::NONE, "fresh second status is absent");
        test.assert(combatant.moves[0].effect1, Effect::NONE, "fresh packed move has no first effect");
        test.assert(combatant.moves[0].effect2, Effect::NONE, "fresh packed move has no second effect");
    }
    suite.addTest(test);
}

namespace battle_damage_test_detail {

inline Move makeMove(Type type, uint8_t power, bool physical,
                     Accuracy accuracy = Accuracy::HUNDRED)
{
    return Move(MoveBitSet{
        static_cast<uint8_t>(type), power, static_cast<uint8_t>(physical),
        static_cast<uint8_t>(accuracy), 0
    });
}

inline battle::Combatant combatant(Type type)
{
    battle::Combatant result = {};
    result.types = DualType(type, Type::NONE);
    result.maxHp = result.hp = result.stats.hp = 100;
    result.stats.attack = 40;
    result.stats.defense = 20;
    result.stats.spcAtk = 30;
    result.stats.spcDef = 15;
    result.moves[0] = makeMove(type, 10, true);
    return result;
}

} // namespace battle_damage_test_detail

void BattleDamageIntegrationTest(TestSuite &suite)
{
    using namespace battle;
    using namespace battle_damage_test_detail;
    Test test(__func__);

    const uint16_t stageExpected[9] = {33, 40, 50, 66, 100, 150, 200, 250, 300};
    for (uint8_t index = 0; index < 9; ++index) {
        const int8_t stage = static_cast<int8_t>(index) - 4;
        test.assert(applyStage(100, stage), stageExpected[index],
                    "nine-entry stage table value");
    }
    test.assert(applyStage(100, -127), static_cast<uint16_t>(33),
                "stage clamps below minus four");
    test.assert(applyStage(100, 127), static_cast<uint16_t>(300),
                "stage clamps above plus four");
    test.assert(applyStage(applyStage(100, 1), -1), static_cast<uint16_t>(100),
                "plus one then minus one preserves base");

    Combatant attacker = combatant(Type::WIND);
    Combatant defender = combatant(Type::SPIRIT);
    test.addToLog("known damage expected=40 from power10 attack40 defense20 half-base STAB");
    test.assert(computeDamage(attacker, defender, 0), static_cast<uint8_t>(40),
                "known damage value");

    attacker = combatant(Type::SPIRIT);
    defender = combatant(Type::SPIRIT);
    attacker.stats.spcAtk = 12;
    defender.stats.spcDef = 6;
    attacker.moves[0] = makeMove(Type::FIRE, 10, false);
    test.assert(computeDamage(attacker, defender, 0), static_cast<uint8_t>(20),
                "special move selects special attack and defense");

    attacker = combatant(Type::SPIRIT);
    defender = combatant(Type::WATER);
    attacker.moves[0] = makeMove(Type::FIRE, 10, true);
    test.assert(computeDamage(attacker, defender, 0), static_cast<uint8_t>(0),
                "type immunity returns zero");
    defender.types = DualType(Type::WIND, Type::NONE);
    test.assert(computeDamage(attacker, defender, 0), static_cast<uint8_t>(40),
                "type double modifier applies");
    defender.types = DualType(Type::WIND, Type::WATER);
    test.assert(computeDamage(attacker, defender, 0), static_cast<uint8_t>(0),
                "dual-type immunity absorbs other effectiveness");

    attacker = combatant(Type::WIND);
    defender = combatant(Type::SPIRIT);
    attacker.status.effects[0] = Effect::BUFTD;
    test.assert(computeDamage(attacker, defender, 0), static_cast<uint8_t>(20),
                "attacker type-down cancels same-type bonus");
    attacker.status.clearEffects();
    defender.status.effects[0] = Effect::DPRSD;
    test.assert(computeDamage(attacker, defender, 0), static_cast<uint8_t>(80),
                "defender type-down is inverted for damage");

    attacker = combatant(Type::WIND);
    defender = combatant(Type::SPIRIT);
    attacker.statMods.setModifier(StatType::ATTACK_M, 1);
    test.assert(computeDamage(attacker, defender, 0), static_cast<uint8_t>(60),
                "attacker stage scales attack term");
    attacker.statMods.clearModifiers();
    defender.statMods.setModifier(StatType::DEFENSE_M, 1);
    test.assert(computeDamage(attacker, defender, 0), static_cast<uint8_t>(26),
                "defender stage scales defense term");

    attacker = combatant(Type::SPIRIT);
    defender = combatant(Type::SPIRIT);
    attacker.stats.attack = 1;
    defender.stats.defense = 255;
    attacker.moves[0] = makeMove(Type::SPIRIT, 1, true);
    test.assert(computeDamage(attacker, defender, 0), static_cast<uint8_t>(1),
                "nonimmune damage floors at one");
    defender.stats.defense = 1;
    test.assert(computeDamage(attacker, defender, 0), static_cast<uint8_t>(1),
                "defense one clamps divisor instead of dividing by zero");

    attacker.stats.attack = 3;
    attacker.moves[0] = makeMove(Type::SPIRIT, 5, true);
    test.assert(computeDamage(attacker, defender, 0), static_cast<uint8_t>(14),
                "half-base rounds down before same-type multiplier");

    attacker = combatant(Type::NONE);
    attacker.stats.attack = 255;
    attacker.moves[0] = makeMove(Type::SPIRIT, 31, true);
    defender = combatant(Type::SPIRIT);
    defender.stats.defense = 1;
    test.assert(computeDamage(attacker, defender, 0), static_cast<uint8_t>(255),
                "large damage saturates at 255 instead of wrapping");

    attacker = combatant(Type::WIND);
    defender = combatant(Type::SPIRIT);
    attacker.moves[0] = makeMove(Type::WIND, 0, true);
    test.assert(computeDamage(attacker, defender, 0), static_cast<uint8_t>(0),
                "zero-power move stays zero");
    test.assert(computeDamage(attacker, defender, 4), static_cast<uint8_t>(0),
                "out-of-range move slot stays zero");
    attacker.moves[0] = makeMove(static_cast<Type>(9), 10, true);
    test.assert(computeDamage(attacker, defender, 0), static_cast<uint8_t>(0),
                "invalid move type stays bounded");

    attacker = combatant(Type::WIND);
    defender = combatant(Type::SPIRIT);
    attacker.moves[0] = makeMove(Type::WIND, 10, true, Accuracy::HUNDRED);
    const uint8_t deterministic = computeDamage(attacker, defender, 0);
    attacker.moves[0] = makeMove(Type::WIND, 10, true, Accuracy::SEVENTY);
    test.assert(computeDamage(attacker, defender, 0), deterministic,
                "accuracy is ignored by deterministic damage");
    for (uint8_t repeat = 0; repeat < 8; ++repeat) {
        test.assert(computeDamage(attacker, defender, 0), deterministic,
                    "critical and variance remain out of scope");
    }

    suite.addTest(test);
}

void BattleSuite(TestRunner &runner) {
    TestSuite suite("Battle data contract suite");
    BattleDataContractTest(suite);
    BattleDamageIntegrationTest(suite);
    runner.addTestSuite(suite);
}
