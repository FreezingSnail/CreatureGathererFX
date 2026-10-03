#pragma once
#include "test.hpp"
#include "../src/engine/battle/BattleState.hpp"
#include "../src/engine/battle/ActionResult.hpp"

#include <stddef.h>
#include <type_traits>

static_assert(sizeof(Move) == 4, "packed move must remain four bytes");
static_assert(sizeof(StatusEffect) == 2, "two status slots must remain two bytes");
static_assert(sizeof(battle::BenchSlot) == 3, "bench carries only ID, level and HP");
static_assert(sizeof(battle::BattleState) <= 100, "native state stays compact");
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

void BattleSuite(TestRunner &runner) {
    TestSuite suite("Battle data contract suite");
    BattleDataContractTest(suite);
    runner.addTestSuite(suite);
}
