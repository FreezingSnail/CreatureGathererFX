#pragma once

#include "test.hpp"
#include "../src/engine/battle/Resolve.hpp"
#include "../src/engine/battle/BattleSetup.hpp"

namespace battle_resolve_test_detail {

uint8_t scriptedValues[16];
uint8_t scriptedLength = 0;
uint8_t scriptedIndex = 0;
uint8_t scriptedCalls = 0;
uint8_t scriptedLastBound = 0;

void script(uint8_t first, uint8_t second = 0, uint8_t third = 0,
            uint8_t fourth = 0)
{
    scriptedValues[0] = first;
    scriptedValues[1] = second;
    scriptedValues[2] = third;
    scriptedValues[3] = fourth;
    scriptedLength = 4;
    scriptedIndex = 0;
    scriptedCalls = 0;
    scriptedLastBound = 0;
}

uint8_t scriptedRoll(uint8_t bound)
{
    ++scriptedCalls;
    scriptedLastBound = bound;
    if (scriptedIndex >= scriptedLength) return 0;
    return scriptedValues[scriptedIndex++];
}

Move makeMove(Type type, uint8_t power, Effect first = Effect::NONE,
              Effect second = Effect::NONE)
{
    Move move(MoveBitSet{
        static_cast<uint8_t>(type), power, 1, 0, 0
    });
    move.effect1 = first;
    move.effect2 = second;
    return move;
}

battle::BattleState stateFixture()
{
    battle::BattleState state = {};
    for (uint8_t side = 0; side < 2; ++side) {
        state.active[side].id = static_cast<uint8_t>(side + 1);
        state.active[side].types = DualType(Type::SPIRIT, Type::NONE);
        state.active[side].level = 1;
        state.active[side].hp = state.active[side].maxHp = 100;
        state.active[side].stats.hp = 100;
        state.active[side].stats.attack = 40;
        state.active[side].stats.defense = 20;
        state.active[side].stats.spcAtk = 40;
        state.active[side].stats.spcDef = 20;
        state.active[side].stats.speed = 10;
        state.active[side].moves[0] = makeMove(Type::SPIRIT, 10);
        state.active[side].moveIds[0] = static_cast<uint8_t>(side + 7);
        state.active[side].moveIds[1] = 255;
        state.active[side].moveIds[2] = 255;
        state.active[side].moveIds[3] = 255;
        state.partyCount[side] = 1;
        state.activeSlot[side] = 0;
    }
    state.gather.need = 12;
    state.gather.fleeTurns = 6;
    return state;
}

uint8_t factCount(const battle::ActionResult &result)
{
    uint8_t count = 0;
    while (count < 4 && result.consequences[count].effect != Effect::NONE)
        ++count;
    return count;
}

void assertSentinels(Test &test, const battle::ActionResult &result)
{
    for (uint8_t slot = 0; slot < 4; ++slot) {
        test.assert(result.consequences[slot].effect, Effect::NONE,
                    "unused result fact effect sentinel");
        test.assert(result.consequences[slot].side, static_cast<uint8_t>(255),
                    "unused result fact side sentinel");
        test.assert(result.consequences[slot].value, static_cast<uint8_t>(0),
                    "unused result fact value sentinel");
    }
}

} // namespace battle_resolve_test_detail

inline void BattleResolveIntegrationTest(TestSuite &suite)
{
    using namespace battle;
    using namespace battle_resolve_test_detail;
    Test test(__func__);
    Rng rng = {scriptedRoll};
    ActionResult result;

    battle::BattleState state = stateFixture();
    TurnPlan plan;
    plan.action[0] = {ActionKind::Attack, 0};
    plan.action[1] = {ActionKind::Attack, 0};
    state.active[0].stats.speed = 100;
    state.active[1].stats.speed = 200;
    test.assert(firstMover(state, plan), Side::Opponent,
                "normal order uses wide unstaged speed");
    state.active[0].statMods.setModifier(StatType::SPEED_M, 3);
    test.assert(firstMover(state, plan), Side::Player,
                "normal order uses staged speed without narrow overflow");
    plan.action[1] = {ActionKind::Switch, 1};
    test.assert(firstMover(state, plan), Side::Opponent,
                "fast opponent switch beats normal player attack");
    plan.action[0] = {ActionKind::Switch, 1};
    plan.action[1] = {ActionKind::Attack, 0};
    test.assert(firstMover(state, plan), Side::Player,
                "fast player switch beats normal opponent attack");
    plan.action[0] = {ActionKind::Attack, 0};
    plan.action[1] = {ActionKind::Attack, 0};
    state.active[1].stats.speed = 250;
    state.active[0].stats.speed = 250;
    state.active[0].statMods.clearModifiers();
    test.assert(firstMover(state, plan), Side::Player,
                "player wins equal priority and speed tie");

    state = stateFixture();
    state.active[0].moves[0] = makeMove(Type::SPIRIT, 10,
                                         Effect::ATKUP, Effect::ATKDWN);
    script(0, 0);
    resolveAction(state, Side::Player, {ActionKind::Attack, 0}, rng, result);
    test.assert(result.kind, ResultKind::Attack, "ordinary attack resolves one action");
    test.assert(result.actor, Side::Player, "ordinary attack names player actor");
    test.assert(result.index, static_cast<uint8_t>(7),
                "attack result names semantic player move ID");
    test.assert(result.effectiveness, Modifier::Double,
                "attack result carries STAB effectiveness");
    test.assert(result.hpBefore[1], static_cast<uint8_t>(100),
                "attack captures opponent HP before impact");
    test.assert(result.hpAfter[1], static_cast<uint8_t>(20),
                "attack writes opponent damage once");
    test.assert(state.active[0].statMods.getModifier(StatType::ATTACK_M), 1,
                "first authored effect targets player");
    test.assert(state.active[1].statMods.getModifier(StatType::ATTACK_M), -1,
                "second authored effect targets opponent");
    test.assert(factCount(result), static_cast<uint8_t>(2),
                "both authored effects become ordered facts");
    test.assert(result.consequences[0].side, static_cast<uint8_t>(Side::Player),
                "first effect fact uses explicit self side");
    test.assert(result.consequences[1].side, static_cast<uint8_t>(Side::Opponent),
                "second effect fact uses explicit opposing side");
    test.assert(scriptedCalls, static_cast<uint8_t>(2),
                "ordinary attack rolls each effect exactly once");

    state = stateFixture();
    state.active[1].hp = 1;
    state.active[0].moves[0] = makeMove(Type::SPIRIT, 10,
                                         Effect::ATKDWN, Effect::SAPPD);
    script(0, 0);
    resolveAction(state, Side::Player, {ActionKind::Attack, 0}, rng, result);
    test.assert(result.flags & OPPONENT_FAINTED, static_cast<uint8_t>(OPPONENT_FAINTED),
                "attack marks only live-to-zero opponent faint");
    test.assert(result.outcome, Outcome::Win,
                "last opponent faint attaches win to current action");
    test.assert(state.over, true, "terminal attack absorbs further resolution");
    test.assert(factCount(result), static_cast<uint8_t>(0),
                "fainted target receives no move effects");
    test.assert(scriptedCalls, static_cast<uint8_t>(0),
                "fainted target consumes no effect rolls");
    const uint8_t terminalHp = state.active[1].hp;
    script(0, 0);
    resolveAction(state, Side::Player, {ActionKind::Attack, 0}, rng, result);
    test.assert(result.kind, ResultKind::None,
                "repeated terminal action is not resolved again");
    test.assert(state.active[1].hp, terminalHp,
                "repeated terminal action preserves HP");
    test.assert(scriptedCalls, static_cast<uint8_t>(0),
                "repeated terminal action consumes no RNG");

    state = stateFixture();
    state.active[0].status.effects[0] = Effect::PINNED;
    script(0);
    resolveAction(state, Side::Player, {ActionKind::Attack, 0}, rng, result);
    test.assert(result.kind, ResultKind::Skip, "PINNED resolves as one skipped action");
    test.assert((result.flags & STATUS_SKIPPED) != 0, true,
                "PINNED sets status-skipped flag");
    test.assert(state.active[1].hp, static_cast<uint8_t>(100),
                "PINNED skip leaves target HP unchanged");
    test.assert(scriptedCalls, static_cast<uint8_t>(1),
                "PINNED consumes one gate roll");
    test.assert(scriptedLastBound, static_cast<uint8_t>(3),
                "PINNED uses one-in-three gate");

    state = stateFixture();
    state.active[0].status.effects[0] = Effect::PINNED;
    state.active[0].status.effects[1] = Effect::CONCUSED;
    script(1, 0);
    resolveAction(state, Side::Player, {ActionKind::Attack, 0}, rng, result);
    test.assert(result.kind, ResultKind::Attack,
                "failed first gate reaches second gate self-hit");
    test.assert((result.flags & SELF_HIT) != 0, true,
                "CONCUSED sets self-hit flag");
    test.assert(state.active[0].hp, static_cast<uint8_t>(20),
                "self-hit damages actor with selected move");
    test.assert(state.active[1].hp, static_cast<uint8_t>(100),
                "self-hit leaves opponent untouched");
    test.assert(factCount(result), static_cast<uint8_t>(0),
                "self-hit applies no move effects");
    test.assert(scriptedCalls, static_cast<uint8_t>(2),
                "isolated gates roll each status slot once");
    test.assert(scriptedLastBound, static_cast<uint8_t>(4),
                "CONCUSED uses one-in-four gate");

    state = stateFixture();
    state.active[0].status.effects[0] = Effect::PINNED;
    script(0);
    resolveAction(state, Side::Player, {ActionKind::Skip, 0}, rng, result);
    test.assert(result.kind, ResultKind::Skip, "explicit skip resolves without gate");
    test.assert(result.flags, static_cast<uint8_t>(0),
                "explicit skip has no status or refusal flag");
    test.assert(scriptedCalls, static_cast<uint8_t>(0),
                "explicit skip consumes no RNG");
    state.active[0].moves[0].effect1 = Effect::ATKUP;
    resolveAction(state, Side::Player, {ActionKind::Attack, 4}, rng, result);
    test.assert(result.kind, ResultKind::Skip, "invalid move becomes refused skip");
    test.assert((result.flags & REFUSED) != 0, true,
                "invalid move records refusal");
    test.assert(state.active[0].statMods.getModifier(StatType::ATTACK_M), 0,
                "invalid move applies no effects");

    state = stateFixture();
    state.active[0].hp = 0;
    state.partyCount[0] = 3;
    state.bench[0][0] = {3, 1, 40};
    state.bench[0][1] = {4, 1, 0};
    test.assert(sideDefeated(state, Side::Player), false,
                "live bench prevents side defeat");
    test.assert(canSwitch(state, Side::Player, 1), true,
                "original live bench slot is switchable");
    test.assert(canSwitch(state, Side::Player, 2), false,
                "dead bench slot is not switchable");
    state.bench[0][0].hp = 0;
    test.assert(sideDefeated(state, Side::Player), true,
                "active plus all bench HP zero defeats side");
    test.assert(canSwitch(state, Side::Player, 0), false,
                "current slot cannot switch to itself");

    state = stateFixture();
    state.active[0].status.effects[0] = Effect::SAPPD;
    state.active[0].status.effects[1] = Effect::INFSED;
    state.active[1].status.effects[0] = Effect::SAPPD;
    state.active[1].status.effects[1] = Effect::INFSED;
    script(0);
    resolveEndTurn(state, rng, false, result);
    test.assert(result.kind, ResultKind::EndTurn,
                "end turn resolves as one compact result");
    test.assert(factCount(result), static_cast<uint8_t>(4),
                "end turn emits four ordered live tick facts");
    test.assert(result.consequences[0].side, static_cast<uint8_t>(Side::Player),
                "end turn ticks player slot zero first");
    test.assert(result.consequences[1].side, static_cast<uint8_t>(Side::Player),
                "end turn ticks player slot one second");
    test.assert(result.consequences[2].side, static_cast<uint8_t>(Side::Opponent),
                "end turn ticks opponent after player");
    test.assert(result.consequences[3].side, static_cast<uint8_t>(Side::Opponent),
                "end turn ticks opponent slot one last");
    test.assert(state.active[0].hp, static_cast<uint8_t>(100),
                "mixed player ticks preserve final HP");
    test.assert(state.active[1].hp, static_cast<uint8_t>(100),
                "mixed opponent ticks preserve final HP");
    test.assert(scriptedCalls, static_cast<uint8_t>(0),
                "end turn tick boundary consumes no RNG");

    state = stateFixture();
    state.active[1].hp = 1;
    state.active[1].status.effects[0] = Effect::SAPPD;
    resolveEndTurn(state, rng, false, result);
    test.assert((result.flags & OPPONENT_FAINTED) != 0, true,
                "tick boundary marks opponent live-to-zero faint");
    test.assert(result.outcome, Outcome::Win,
                "tick boundary attaches win after opponent KO");
    test.assert(state.over, true, "tick terminal state becomes absorbing");
    resolveEndTurn(state, rng, false, result);
    test.assert(result.kind, ResultKind::EndTurn,
                "repeated terminal end turn stays a result boundary");
    test.assert(result.outcome, Outcome::None,
                "repeated terminal end turn does not replay outcome");
    test.assert(factCount(result), static_cast<uint8_t>(0),
                "repeated terminal end turn emits no duplicate tick facts");

    state = stateFixture();
    state.active[0].hp = state.active[1].hp = 1;
    state.active[0].status.effects[0] = Effect::SAPPD;
    state.active[1].status.effects[0] = Effect::SAPPD;
    resolveEndTurn(state, rng, false, result);
    test.assert(result.outcome, Outcome::Lose,
                "player loss wins terminal priority when both faint");
    test.assert((result.flags & PLAYER_FAINTED) != 0, true,
                "both-faint end turn marks player transition");
    test.assert((result.flags & OPPONENT_FAINTED) != 0, true,
                "both-faint end turn marks opponent transition");

    state = stateFixture();
    result.kind = ResultKind::Attack;
    result.flags = 255;
    result.consequences[0] = {Effect::ATKUP, 0, 3};
    resolveAction(state, Side::Player, {ActionKind::Skip, 0}, rng, result);
    test.assert(result.index, static_cast<uint8_t>(255),
                "resolver resets caller semantic index");
    test.assert(result.flags, static_cast<uint8_t>(0),
                "resolver resets caller flags");
    assertSentinels(test, result);

    state = stateFixture();
    script(0);
    resolveAction(state, Side::Player, {ActionKind::Switch, 1}, rng, result);
    test.assert(result.kind, ResultKind::Switch,
                "switch remains an explicit setup-owned result branch");
    test.assert((result.flags & REFUSED) != 0, true,
                "resolver does not perform an FX switch transition");
    test.assert(scriptedCalls, static_cast<uint8_t>(0),
                "switch dispatch consumes no resolver RNG");

    suite.addTest(test);
}

inline void BattleResolveSuite(TestRunner &runner)
{
    TestSuite suite("Battle resolution integration");
    BattleResolveIntegrationTest(suite);
    runner.addTestSuite(suite);
}
