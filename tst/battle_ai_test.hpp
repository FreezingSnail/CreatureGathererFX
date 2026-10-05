#pragma once

#include "test.hpp"
#include "../src/engine/battle/Ai.hpp"

namespace battle_ai_test_detail {

Move makeMove(Type type, uint8_t power)
{
    return Move(MoveBitSet{
        static_cast<uint8_t>(type), power, 1, 0, 0
    });
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
        state.partyCount[side] = 1;
        state.activeSlot[side] = 0;
        for (uint8_t slot = 0; slot < 4; ++slot) {
            state.active[side].moveIds[slot] = 255;
        }
    }
    return state;
}

void setMove(battle::BattleState &state, battle::Side side, uint8_t slot,
             uint8_t id, Type type, uint8_t power)
{
    const uint8_t index = static_cast<uint8_t>(side);
    state.active[index].moveIds[slot] = id;
    state.active[index].moves[slot] = makeMove(type, power);
}

void clearMoves(battle::BattleState &state, battle::Side side)
{
    const uint8_t index = static_cast<uint8_t>(side);
    for (uint8_t slot = 0; slot < 4; ++slot) {
        state.active[index].moveIds[slot] = 255;
        state.active[index].moves[slot] = Move();
    }
}

void assertSkip(Test &test, const battle::BattleAction &action,
                const std::string &message)
{
    test.assert(action.kind, battle::ActionKind::Skip, message + " kind");
    test.assert(action.index, static_cast<uint8_t>(255), message + " index");
}

} // namespace battle_ai_test_detail

void BattleAiIntegrationTest(TestSuite &suite)
{
    using namespace battle;
    using namespace battle_ai_test_detail;
    Test test(__func__);

    battle::BattleState state = stateFixture();
    // Player slot zero is intentionally stronger; opponent slot two must be
    // evaluated against the player's combatant, not through the legacy inverse.
    setMove(state, Side::Player, 0, 10, Type::SPIRIT, 31);
    setMove(state, Side::Opponent, 0, 20, Type::SPIRIT, 5);
    setMove(state, Side::Opponent, 2, 21, Type::SPIRIT, 20);
    BattleAction action = chooseAction(state, Side::Opponent);
    test.assert(action.kind, ActionKind::Attack,
                "opponent chooses an attack action");
    test.assert(action.index, static_cast<uint8_t>(2),
                "opponent evaluates its own strongest move slot");

    const BattleAction repeated = chooseAction(state, Side::Opponent);
    test.assert(repeated.kind, action.kind,
                "identical state keeps deterministic action kind");
    test.assert(repeated.index, action.index,
                "identical state keeps deterministic action slot");

    clearMoves(state, Side::Opponent);
    setMove(state, Side::Opponent, 0, 30, Type::SPIRIT, 10);
    setMove(state, Side::Opponent, 1, 31, Type::SPIRIT, 10);
    action = chooseAction(state, Side::Opponent);
    test.assert(action.index, static_cast<uint8_t>(0),
                "equal damage breaks ties by lowest slot");

    clearMoves(state, Side::Opponent);
    state.active[static_cast<uint8_t>(Side::Player)].types =
        DualType(Type::WATER, Type::NONE);
    setMove(state, Side::Opponent, 1, 0, Type::FIRE, 4);
    setMove(state, Side::Opponent, 3, 32, Type::FIRE, 20);
    action = chooseAction(state, Side::Opponent);
    test.assert(action.index, static_cast<uint8_t>(3),
                "immune matchup falls back to highest move power");
    test.assert(action.kind, ActionKind::Attack,
                "zero-damage fallback still acts instead of skipping");

    clearMoves(state, Side::Opponent);
    setMove(state, Side::Opponent, 0, 255, Type::SPIRIT, 31);
    setMove(state, Side::Opponent, 1, 0, Type::SPIRIT, 5);
    action = chooseAction(state, Side::Opponent);
    test.assert(action.index, static_cast<uint8_t>(1),
                "absent slot is ignored while semantic move ID zero remains legal");

    clearMoves(state, Side::Opponent);
    assertSkip(test, chooseAction(state, Side::Opponent),
               "empty move list returns skip");

    state.active[static_cast<uint8_t>(Side::Player)].hp = 0;
    assertSkip(test, chooseAction(state, Side::Opponent),
               "fainted target returns skip");
    state.active[static_cast<uint8_t>(Side::Player)].hp = 100;
    state.active[static_cast<uint8_t>(Side::Opponent)].hp = 0;
    assertSkip(test, chooseAction(state, Side::Opponent),
               "fainted actor returns skip");
    state.active[static_cast<uint8_t>(Side::Opponent)].hp = 100;

    state.partyCount[static_cast<uint8_t>(Side::Opponent)] = 0;
    assertSkip(test, chooseAction(state, Side::Opponent),
               "empty actor party returns skip");
    state.partyCount[static_cast<uint8_t>(Side::Opponent)] = 1;
    state.activeSlot[static_cast<uint8_t>(Side::Opponent)] = 1;
    assertSkip(test, chooseAction(state, Side::Opponent),
               "invalid active party slot returns skip");
    state.activeSlot[static_cast<uint8_t>(Side::Opponent)] = 0;

    assertSkip(test, chooseAction(state, static_cast<Side>(2)),
               "invalid side returns skip");
    state.over = true;
    assertSkip(test, chooseAction(state, Side::Opponent),
               "terminal state returns skip");

    suite.addTest(test);
}

void BattleAiSwitchRiskTest(TestSuite &suite)
{
    using namespace battle;
    using namespace battle_ai_test_detail;
    Test test(__func__);
    battle::BattleState state = stateFixture();
    state.trainer = true;
    state.partyCount[1] = 3;
    state.active[0].hp = state.active[0].maxHp = 255;
    state.active[1].hp = state.active[1].maxHp = 255;
    state.active[1].stats.defense = 2;
    setMove(state, Side::Player, 0, 10, Type::SPIRIT, 31);
    setMove(state, Side::Opponent, 0, 20, Type::SPIRIT, 1);
    state.bench[1][0] = {3, 1, 255, DualType(Type::SPIRIT), 14, 14};
    state.bench[1][1] = {4, 1, 254, DualType(Type::SPIRIT), 14, 14};
    BattleAction action = chooseAction(state, Side::Opponent);
    test.assert(action.kind, ActionKind::Switch, "high-HP trainer switches to surviving defense");
    test.assert(action.index, 1, "risk products above signed 16-bit range retain best candidate");
    state.bench[1][1].hp = 255;
    test.assert(chooseAction(state, Side::Opponent).index, 1,
                "equal high-HP risk preserves original slot tie order");
    state.switchLockMask = 2;
    test.assert(chooseAction(state, Side::Opponent).kind, ActionKind::Attack,
                "switch lock preserves required intervening attack");
    state.switchLockMask = 0;
    state.active[0].hp = 1;
    test.assert(chooseAction(state, Side::Opponent).kind, ActionKind::Attack,
                "lethal action still takes priority over defensive switching");
    suite.addTest(test);
}

void BattleAiSuite(TestRunner &runner)
{
    TestSuite suite("Battle AI integration");
    BattleAiIntegrationTest(suite);
    BattleAiSwitchRiskTest(suite);
    runner.addTestSuite(suite);
}
