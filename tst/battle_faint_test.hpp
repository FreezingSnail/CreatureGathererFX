#pragma once

#include "test.hpp"
#include "../src/engine/battle/BattleSetup.hpp"
#include "../src/engine/battle/Resolve.hpp"
#include "../src/lib/FxReadCounter.hpp"

namespace battle_faint_test_detail {

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
        state.active[side].moves[0] = Move(MoveBitSet{
            static_cast<uint8_t>(Type::SPIRIT), 10, 1, 0, 0
        });
        state.active[side].moveIds[0] = static_cast<uint8_t>(side + 7);
        for (uint8_t slot = 1; slot < 4; ++slot) {
            state.active[side].moveIds[slot] = 255;
        }
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
    while (count < 4 && result.consequences[count].effect != Effect::NONE) {
        ++count;
    }
    return count;
}

void assertCoreState(Test &test, const battle::BattleState &actual,
                     const battle::BattleState &expected,
                     const char *prefix)
{
    for (uint8_t side = 0; side < 2; ++side) {
        test.assert(actual.active[side].id, expected.active[side].id,
                    prefix);
        test.assert(actual.active[side].hp, expected.active[side].hp,
                    prefix);
        test.assert(actual.active[side].maxHp, expected.active[side].maxHp,
                    prefix);
        test.assert(actual.activeSlot[side], expected.activeSlot[side],
                    prefix);
        test.assert(actual.partyCount[side], expected.partyCount[side],
                    prefix);
        for (uint8_t slot = 0; slot < PARTY_SIZE - 1; ++slot) {
            test.assert(actual.bench[side][slot].id,
                        expected.bench[side][slot].id, prefix);
            test.assert(actual.bench[side][slot].level,
                        expected.bench[side][slot].level, prefix);
            test.assert(actual.bench[side][slot].hp,
                        expected.bench[side][slot].hp, prefix);
        }
    }
    test.assert(actual.over, expected.over, prefix);
}

} // namespace battle_faint_test_detail

inline void BattleFaintSwitchIntegrationTest(TestSuite &suite)
{
    using namespace battle;
    using namespace battle_faint_test_detail;
    Test test(__func__);
    Rng rng = {nullptr};
    ActionResult result;

    // Pure resolution is deterministic and never performs an FX lookup.
    battle::BattleState first = stateFixture();
    battle::BattleState second = first;
    first.active[1].hp = second.active[1].hp = 30;
    FxReadCounter::resetFrame();
    resolveAction(first, Side::Player, {ActionKind::Attack, 0}, rng, result);
    ActionResult firstResult = result;
    test.assert(FxReadCounter::count(), static_cast<uint8_t>(0),
                "faint resolution performs no FX reads");
    test.assert(FxReadCounter::markUpdate(), true,
                "faint resolution leaves the frame read budget clean");
    resetActionResult(result);
    resolveAction(second, Side::Player, {ActionKind::Attack, 0}, rng, result);
    test.assert(result.kind, firstResult.kind, "identical state gives identical result kind");
    test.assert(result.index, firstResult.index, "identical state gives identical result index");
    test.assert(result.flags, firstResult.flags, "identical state gives identical result flags");
    test.assert(result.outcome, firstResult.outcome,
                "identical state gives identical terminal outcome");
    test.assert(second.active[1].hp, first.active[1].hp,
                "identical state gives identical HP transition");

    // A one-creature opponent ends immediately; no empty bench slot is used.
    battle::BattleState state = stateFixture();
    state.active[1].hp = 1;
    resolveAction(state, Side::Player, {ActionKind::Attack, 0}, rng, result);
    test.assert((result.flags & OPPONENT_FAINTED) != 0, true,
                "last opponent active marks a live-to-zero faint");
    test.assert(result.outcome, Outcome::Win,
                "last opponent faint resolves to Win");
    test.assert(state.over, true, "last opponent faint makes state terminal");
    test.assert(canSwitch(state, Side::Opponent, 1), false,
                "one-creature opponent has no empty replacement");

    // A real three-slot party leaves the lowest live original slot for a
    // separate forced switch result after faint playback.
    state = stateFixture();
    state.partyCount[static_cast<uint8_t>(Side::Opponent)] = 3;
    state.bench[static_cast<uint8_t>(Side::Opponent)][0] = {3, 1, 0};
    state.bench[static_cast<uint8_t>(Side::Opponent)][1] = {4, 1, 40};
    state.active[1].status.effects[0] = Effect::PINNED;
    state.active[1].statMods.setModifier(StatType::ATTACK_M, 2);
    state.active[1].hp = 1;
    resolveAction(state, Side::Player, {ActionKind::Attack, 0}, rng, result);
    test.assert((result.flags & OPPONENT_FAINTED) != 0, true,
                "multi-party opponent faint marks exactly the transition");
    test.assert(result.outcome, Outcome::None,
                "live opponent bench prevents premature Win");
    test.assert(state.over, false, "live opponent bench keeps battle active");
    test.assert(canSwitch(state, Side::Opponent, 1), false,
                "dead original opponent slot is refused");
    test.assert(canSwitch(state, Side::Opponent, 2), true,
                "live original opponent slot is selectable");

    FxReadCounter::resetFrame();
    test.assert(applySwitch(state, Side::Opponent, 2, true, result), true,
                "forced opponent replacement succeeds from original slot");
    test.assert(result.kind, ResultKind::Switch,
                "forced replacement emits a separate Switch result");
    test.assert(result.flags, static_cast<uint8_t>(FORCED_SWITCH),
                "forced replacement marks forced feedback");
    test.assert(result.hpBefore[static_cast<uint8_t>(Side::Opponent)],
                static_cast<uint8_t>(0), "replacement result preserves faint HP before");
    test.assert(result.hpAfter[static_cast<uint8_t>(Side::Opponent)],
                static_cast<uint8_t>(40), "replacement result restores bench HP");
    test.assert(result.speciesBefore[static_cast<uint8_t>(Side::Player)],
                static_cast<uint8_t>(1), "replacement result keeps other species");
    test.assert(result.hpAfter[static_cast<uint8_t>(Side::Player)],
                static_cast<uint8_t>(100), "replacement result keeps other HP");
    test.assert(state.activeSlot[static_cast<uint8_t>(Side::Opponent)],
                static_cast<uint8_t>(2), "replacement stores original active slot");
    test.assert(state.active[static_cast<uint8_t>(Side::Opponent)].hp,
                static_cast<uint8_t>(40), "replacement restores incoming HP");
    test.assert(state.active[static_cast<uint8_t>(Side::Opponent)].status.effects[0],
                Effect::NONE, "replacement clears incoming statuses");
    test.assert(state.active[static_cast<uint8_t>(Side::Opponent)].statMods
                    .getModifier(StatType::ATTACK_M), 0,
                "replacement clears incoming stages");
    test.assert(state.bench[static_cast<uint8_t>(Side::Opponent)][0].hp,
                static_cast<uint8_t>(0), "replacement keeps dead original slot first");

    // A fainted player with a live bench waits for choice; without one, Lose.
    state = stateFixture();
    state.partyCount[static_cast<uint8_t>(Side::Player)] = 2;
    state.bench[static_cast<uint8_t>(Side::Player)][0] = {3, 1, 40};
    state.active[static_cast<uint8_t>(Side::Player)].hp = 1;
    resolveAction(state, Side::Opponent, {ActionKind::Attack, 0}, rng, result);
    test.assert((result.flags & PLAYER_FAINTED) != 0, true,
                "player active faint is reported once");
    test.assert(result.outcome, Outcome::None,
                "live player bench requires a replacement choice");
    test.assert(state.over, false, "live player bench prevents premature Lose");

    state = stateFixture();
    state.active[static_cast<uint8_t>(Side::Player)].hp = 1;
    resolveAction(state, Side::Opponent, {ActionKind::Attack, 0}, rng, result);
    test.assert((result.flags & PLAYER_FAINTED) != 0, true,
                "last player active marks a live-to-zero faint");
    test.assert(result.outcome, Outcome::Lose,
                "last player faint resolves to Lose");
    test.assert(state.over, true, "last player faint makes state terminal");

    state = stateFixture();
    state.active[static_cast<uint8_t>(Side::Player)].hp = 0;
    resolveAction(state, Side::Player, {ActionKind::Attack, 0}, rng, result);
    test.assert(result.kind, ResultKind::Skip,
                "already-fainted actor cannot resolve an action");
    test.assert(result.outcome, Outcome::Lose,
                "no-live player action reports terminal Lose");

    // Invalid, current, and dead original-slot choices refuse without mutation.
    state = stateFixture();
    state.partyCount[static_cast<uint8_t>(Side::Player)] = 3;
    state.activeSlot[static_cast<uint8_t>(Side::Player)] = 1;
    state.active[static_cast<uint8_t>(Side::Player)].id = 2;
    state.active[static_cast<uint8_t>(Side::Player)].hp = 17;
    state.bench[static_cast<uint8_t>(Side::Player)][0] = {3, 1, 40};
    state.bench[static_cast<uint8_t>(Side::Player)][1] = {4, 1, 0};
    const battle::BattleState beforeRefused = state;
    const uint8_t refusedSlots[] = {1, 2, 3};
    for (uint8_t slot : refusedSlots) {
        resetActionResult(result);
        test.assert(applySwitch(state, Side::Player, slot, false, result), false,
                    "invalid/current/dead switch is refused");
        test.assert((result.flags & REFUSED) != 0, true,
                    "refused switch reports refusal");
        assertCoreState(test, state, beforeRefused,
                        "refused switch leaves battle state untouched");
    }

    // Outgoing and incoming HP round-trip by original slot; incoming state is reset.
    state.active[static_cast<uint8_t>(Side::Player)].status.effects[0] = Effect::PINNED;
    state.active[static_cast<uint8_t>(Side::Player)].statMods
        .setModifier(StatType::ATTACK_M, 2);
    test.assert(applySwitch(state, Side::Player, 0, false, result), true,
                "live player bench switch succeeds");
    test.assert(state.active[static_cast<uint8_t>(Side::Player)].hp, 40,
                "switch loads preserved incoming HP");
    test.assert(state.active[static_cast<uint8_t>(Side::Player)].status.effects[0],
                Effect::NONE, "player incoming status resets");
    test.assert(state.active[static_cast<uint8_t>(Side::Player)].statMods
                    .getModifier(StatType::ATTACK_M), 0,
                "player incoming stages reset");
    test.assert(applySwitch(state, Side::Player, 1, false, result), true,
                "switch back to outgoing original slot succeeds");
    test.assert(state.active[static_cast<uint8_t>(Side::Player)].hp, 17,
                "switch back round-trips outgoing HP");

    // SAPPD followed by INFSED cannot revive a zero HP actor; each faint flag is one bit.
    state = stateFixture();
    state.active[0].hp = state.active[1].hp = 1;
    state.active[0].status.effects[0] = Effect::SAPPD;
    state.active[0].status.effects[1] = Effect::INFSED;
    state.active[1].status.effects[0] = Effect::SAPPD;
    state.active[1].status.effects[1] = Effect::INFSED;
    resolveEndTurn(state, rng, false, result);
    test.assert((result.flags & PLAYER_FAINTED) != 0, true,
                "mixed player tick reports one faint");
    test.assert((result.flags & OPPONENT_FAINTED) != 0, true,
                "mixed opponent tick reports one faint");
    test.assert(factCount(result), static_cast<uint8_t>(2),
                "double ticks emit only successful pre-faint facts");
    test.assert(state.active[0].hp, static_cast<uint8_t>(0),
                "dead player is not revived by later INFSED");
    test.assert(state.active[1].hp, static_cast<uint8_t>(0),
                "dead opponent is not revived by later INFSED");
    test.assert(result.outcome, Outcome::Lose,
                "mixed double faint uses player-loss terminal priority");

    suite.addTest(test);
}

inline void BattleFaintSwitchSuite(TestRunner &runner)
{
    TestSuite suite("Battle faint and switch integration");
    BattleFaintSwitchIntegrationTest(suite);
    runner.addTestSuite(suite);
}
