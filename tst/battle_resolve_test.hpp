#pragma once

#include "test.hpp"
#include "../src/engine/battle/Resolve.hpp"
#include "../src/engine/battle/BattleSetup.hpp"
#include "../src/lib/FxReadCounter.hpp"

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
    test.assert((result.flags & PHYSICAL_MOVE) != 0, true,
                "attack result carries its visual move class");
    test.assert(result.effectiveness, Modifier::Double,
                "attack result carries STAB effectiveness");
    test.assert(result.hpBefore[1], static_cast<uint8_t>(100),
                "attack captures opponent HP before impact");
    test.assert(result.hpAfter[1], static_cast<uint8_t>(60),
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
    test.assert(state.active[0].hp, static_cast<uint8_t>(60),
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

inline void BattleEscapeIntegrationTest(TestSuite &suite)
{
    using namespace battle;
    using namespace battle_resolve_test_detail;
    Test test(__func__);
    Rng rng = {scriptedRoll};
    ActionResult result;

    battle::BattleState state = stateFixture();
    state.active[0].hp = 37;
    state.active[1].hp = 83;
    state.gather.progress = 11;
    state.gather.fleeTurns = 0;
    const battle::BattleState wildBeforeEscape = state;
    result.kind = ResultKind::Attack;
    result.flags = 255;
    result.consequences[0] = {Effect::ATKUP, 0, 3};
    script(0);
    FxReadCounter::resetFrame();
    resolveAction(state, Side::Player, {ActionKind::Escape, 0}, rng, result);
    test.assert(result.kind, ResultKind::Escape,
                "wild escape resolves as one terminal action");
    test.assert(result.actor, Side::Player, "wild escape names player actor");
    test.assert(result.index, static_cast<uint8_t>(255),
                "escape result keeps non-attack index sentinel");
    test.assert(result.flags, static_cast<uint8_t>(0),
                "wild escape has no refusal or faint flags");
    test.assert(result.outcome, Outcome::Escaped,
                "wild escape reports Escaped outcome");
    test.assert(result.speciesBefore[0], wildBeforeEscape.active[0].id,
                "escape preserves player species before presentation");
    test.assert(result.speciesBefore[1], wildBeforeEscape.active[1].id,
                "escape preserves opponent species before presentation");
    test.assert(result.hpBefore[0], static_cast<uint8_t>(37),
                "escape captures player HP before terminal result");
    test.assert(result.hpAfter[0], static_cast<uint8_t>(37),
                "escape leaves player HP unchanged");
    test.assert(result.hpBefore[1], static_cast<uint8_t>(83),
                "escape captures opponent HP before terminal result");
    test.assert(result.hpAfter[1], static_cast<uint8_t>(83),
                "escape leaves opponent HP unchanged");
    test.assert(result.progressBefore, static_cast<uint8_t>(11),
                "escape captures gather progress before terminal result");
    test.assert(result.progressAfter, static_cast<uint8_t>(11),
                "escape leaves gather progress unchanged");
    test.assert(state.active[0].hp, wildBeforeEscape.active[0].hp,
                "wild escape preserves active player HP");
    test.assert(state.active[1].hp, wildBeforeEscape.active[1].hp,
                "wild escape preserves active opponent HP");
    test.assert(state.active[0].id, wildBeforeEscape.active[0].id,
                "wild escape preserves active player state");
    test.assert(state.active[1].id, wildBeforeEscape.active[1].id,
                "wild escape preserves active opponent state");
    test.assert(state.gather.fleeTurns, static_cast<uint8_t>(0),
                "wild escape does not underflow zero flee countdown");
    test.assert(state.over, true,
                "wild escape marks battle terminal immediately");
    test.assert(scriptedCalls, static_cast<uint8_t>(0),
                "wild escape consumes no RNG or flee roll");
    test.assert(FxReadCounter::count(), static_cast<uint8_t>(0),
                "wild escape performs no FX reads");
    test.assert(FxReadCounter::markUpdate(), true,
                "wild escape leaves native read budget clean");
    assertSentinels(test, result);

    const uint8_t escapedPlayerHp = state.active[0].hp;
    const uint8_t escapedOpponentHp = state.active[1].hp;
    script(0);
    FxReadCounter::resetFrame();
    resolveAction(state, Side::Opponent, {ActionKind::Attack, 0}, rng, result);
    test.assert(result.kind, ResultKind::None,
                "terminal wild escape cancels the opposing action");
    test.assert(state.active[0].hp, escapedPlayerHp,
                "cancelled opposing action preserves player HP");
    test.assert(state.active[1].hp, escapedOpponentHp,
                "cancelled opposing action preserves opponent HP");
    test.assert(state.over, true,
                "terminal wild escape remains absorbing");
    test.assert(scriptedCalls, static_cast<uint8_t>(0),
                "cancelled opposing action consumes no RNG");
    test.assert(FxReadCounter::count(), static_cast<uint8_t>(0),
                "cancelled opposing action performs no FX reads");

    state = stateFixture();
    state.trainer = true;
    state.active[0].hp = 100;
    state.active[1].hp = 100;
    state.gather.fleeTurns = 0;
    const battle::BattleState trainerBeforeEscape = state;
    script(127);
    FxReadCounter::resetFrame();
    resolveAction(state, Side::Player, {ActionKind::Escape, 0}, rng, result);
    test.assert(result.kind, ResultKind::Escape,
                "trainer escape resolves as one refused action");
    test.assert(result.actor, Side::Player, "trainer escape names player actor");
    test.assert(result.index, static_cast<uint8_t>(255),
                "trainer escape keeps non-attack index sentinel");
    test.assert((result.flags & REFUSED) != 0, true,
                "trainer escape reports refusal");
    test.assert(result.outcome, Outcome::None,
                "trainer escape has no terminal outcome");
    test.assert(state.over, false,
                "trainer escape leaves battle active");
    test.assert(state.active[0].hp, trainerBeforeEscape.active[0].hp,
                "trainer refusal leaves player HP unchanged");
    test.assert(state.active[1].hp, trainerBeforeEscape.active[1].hp,
                "trainer refusal leaves opponent HP unchanged");
    test.assert(state.gather.fleeTurns, static_cast<uint8_t>(0),
                "trainer refusal does not underflow zero flee countdown");
    test.assert(scriptedCalls, static_cast<uint8_t>(0),
                "trainer refusal consumes no RNG or flee roll");
    test.assert(FxReadCounter::count(), static_cast<uint8_t>(0),
                "trainer refusal performs no FX reads");
    test.assert(FxReadCounter::markUpdate(), true,
                "trainer refusal leaves native read budget clean");
    assertSentinels(test, result);

    script(0);
    resolveAction(state, Side::Opponent, {ActionKind::Attack, 0}, rng, result);
    test.assert(result.kind, ResultKind::Attack,
                "trainer refusal consumes only player action");
    test.assert(state.active[0].hp < trainerBeforeEscape.active[0].hp, true,
                "opponent acts after trainer escape refusal");
    test.assert(state.over, false,
                "nonterminal opposing action keeps trainer battle active");

    suite.addTest(test);
}

inline void BattleGatherIntegrationTest(TestSuite &suite)
{
    using namespace battle;
    using namespace battle_resolve_test_detail;
    Test test(__func__);
    Rng rng = {scriptedRoll};
    ActionResult result;

    // Level 12 uses the authored need formula's 14-point middle case. Tier 1
    // advances one point and never overshoots a ready threshold.
    battle::BattleState state = stateFixture();
    state.gatherable = true;
    state.active[1].level = 12;
    state.gather.need = 14;
    state.gather.tierRate = 1;
    script(127);
    FxReadCounter::resetFrame();
    for (uint8_t turn = 0; turn < 14; ++turn) {
        resolveAction(state, Side::Player, {ActionKind::Gather, 0}, rng,
                      result);
    }
    test.assert(result.kind, ResultKind::Gather,
                "ordinary wild gather emits one Gather result");
    test.assert(result.index, static_cast<uint8_t>(255),
                "gather keeps the non-attack index sentinel");
    test.assert(result.progressBefore, static_cast<uint8_t>(13),
                "gather records progress before its final tier increment");
    test.assert(result.progressAfter, static_cast<uint8_t>(14),
                "tier one reaches the level twelve need");
    test.assert(state.gather.progress, static_cast<uint8_t>(14),
                "gather progress persists in battle state");
    test.assert(result.flags, static_cast<uint8_t>(0),
                "ordinary gather is not refused");
    test.assert(result.outcome, Outcome::None,
                "ready gather does not terminate before end turn");
    test.assert(scriptedCalls, static_cast<uint8_t>(0),
                "ordinary gather consumes no RNG without status gates");
    test.assert(FxReadCounter::count(), static_cast<uint8_t>(0),
                "gather resolution performs no FX reads");
    test.assert(FxReadCounter::markUpdate(), true,
                "gather resolution leaves the native read budget clean");

    // Tier 4 saturates exactly at a short threshold, including an over-ready
    // call; progress and caller-owned HP are never reconstructed.
    state = stateFixture();
    state.gatherable = true;
    state.gather.need = 4;
    state.gather.tierRate = 4;
    state.gather.progress = 0;
    state.active[0].hp = 63;
    resolveAction(state, Side::Player, {ActionKind::Gather, 0}, rng, result);
    test.assert(result.progressBefore, static_cast<uint8_t>(0),
                "tier four records zero progress before impact");
    test.assert(result.progressAfter, static_cast<uint8_t>(4),
                "tier four reaches its need in one action");
    test.assert(state.active[0].hp, static_cast<uint8_t>(63),
                "gather preserves persistent player HP");
    resolveAction(state, Side::Player, {ActionKind::Gather, 0}, rng, result);
    test.assert(result.progressBefore, static_cast<uint8_t>(4),
                "repeated ready gather records the saturated before value");
    test.assert(result.progressAfter, static_cast<uint8_t>(4),
                "repeated ready gather does not overflow progress");

    // Non-wild and non-gatherable requests refuse without mutation, RNG, or
    // an FX lookup.
    state = stateFixture();
    const uint8_t refusedProgress = state.gather.progress;
    script(0);
    FxReadCounter::resetFrame();
    resolveAction(state, Side::Player, {ActionKind::Gather, 0}, rng, result);
    test.assert(result.kind, ResultKind::Gather,
                "non-gatherable request remains a Gather result boundary");
    test.assert((result.flags & REFUSED) != 0, true,
                "non-gatherable gather is refused");
    test.assert(result.progressBefore, refusedProgress,
                "refused gather records unchanged progress before");
    test.assert(result.progressAfter, refusedProgress,
                "refused gather records unchanged progress after");
    test.assert(state.gather.progress, refusedProgress,
                "refused gather leaves progress unchanged");
    test.assert(scriptedCalls, static_cast<uint8_t>(0),
                "refused gather consumes no RNG");
    test.assert(FxReadCounter::count(), static_cast<uint8_t>(0),
                "refused gather performs no FX reads");
    test.assert(FxReadCounter::markUpdate(), true,
                "refused gather leaves the native read budget clean");

    state.gatherable = true;
    state.trainer = true;
    resolveAction(state, Side::Player, {ActionKind::Gather, 0}, rng, result);
    test.assert((result.flags & REFUSED) != 0, true,
                "trainer gather is refused even when gatherable is set");
    test.assert(state.gather.progress, refusedProgress,
                "trainer refusal leaves progress unchanged");

    // A ready gather still pays the opponent action and all end-turn ticks;
    // only the pending end-turn result absorbs into Gathered.
    state = stateFixture();
    state.gatherable = true;
    state.gather.need = 4;
    state.gather.tierRate = 4;
    state.gather.fleeTurns = 6;
    state.active[0].status.effects[0] = Effect::SAPPD;
    resolveAction(state, Side::Player, {ActionKind::Gather, 0}, rng, result);
    test.assert(result.progressAfter, static_cast<uint8_t>(4),
                "ready gather records completion before opponent action");
    const uint8_t playerHpBeforeOpponent = state.active[0].hp;
    resolveAction(state, Side::Opponent, {ActionKind::Attack, 0}, rng, result);
    test.assert(result.kind, ResultKind::Attack,
                "opponent still resolves after a ready gather");
    test.assert(state.active[0].hp < playerHpBeforeOpponent, true,
                "ready gather still suffers opponent damage");
    const uint8_t playerHpBeforeTick = state.active[0].hp;
    resolveEndTurn(state, rng, true, result);
    test.assert(result.kind, ResultKind::EndTurn,
                "ready gather completes at the end-turn boundary");
    test.assert(result.outcome, Outcome::Gathered,
                "surviving ready gather reports Gathered");
    test.assert(factCount(result), static_cast<uint8_t>(1),
                "ready gather still emits its ordered status tick fact");
    test.assert(state.active[0].hp < playerHpBeforeTick, true,
                "ready gather still pays the end-turn status tick");
    test.assert(state.gather.fleeTurns, static_cast<uint8_t>(6),
                "acquisition absorbs before countdown mutation");
    test.assert(state.over, true,
                "Gathered makes the terminal state absorbing");

    // Terminal priority: either KO beats pending acquisition; acquisition
    // beats flee when both sides remain live.
    state = stateFixture();
    state.gatherable = true;
    state.gather.fleeTurns = 1;
    state.active[0].hp = 1;
    state.active[0].status.effects[0] = Effect::SAPPD;
    resolveEndTurn(state, rng, true, result);
    test.assert(result.outcome, Outcome::Lose,
                "player KO beats pending acquisition");
    test.assert(state.over, true, "player KO absorbs the pending gather");
    test.assert(state.gather.fleeTurns, static_cast<uint8_t>(1),
                "player KO does not tick the flee countdown");

    state = stateFixture();
    state.gatherable = true;
    state.gather.fleeTurns = 1;
    state.active[1].hp = 1;
    state.active[1].status.effects[0] = Effect::SAPPD;
    resolveEndTurn(state, rng, true, result);
    test.assert(result.outcome, Outcome::Win,
                "opponent KO beats pending acquisition");

    state = stateFixture();
    state.gatherable = true;
    state.gather.fleeTurns = 1;
    resolveEndTurn(state, rng, true, result);
    test.assert(result.outcome, Outcome::Gathered,
                "pending acquisition beats flee expiry");
    test.assert(state.gather.fleeTurns, static_cast<uint8_t>(1),
                "pending acquisition leaves countdown untouched");

    // Flee decrements once per live end-turn call, saturates at zero, and a
    // terminal repeat neither ticks nor underflows.
    state = stateFixture();
    state.gatherable = true;
    state.gather.fleeTurns = 2;
    state.active[0].hp = 77;
    script(0);
    resolveEndTurn(state, rng, false, result);
    test.assert(result.outcome, Outcome::None,
                "non-expired countdown keeps battle active");
    test.assert(state.gather.fleeTurns, static_cast<uint8_t>(1),
                "first end turn decrements flee once");
    test.assert(factCount(result), static_cast<uint8_t>(0),
                "flee-only end turn has no duplicate tick facts");
    test.assert(scriptedCalls, static_cast<uint8_t>(0),
                "flee countdown consumes no RNG");
    resolveEndTurn(state, rng, false, result);
    test.assert(result.outcome, Outcome::Fled,
                "zero countdown reports Fled");
    test.assert(state.gather.fleeTurns, static_cast<uint8_t>(0),
                "flee countdown reaches zero without underflow");
    const uint8_t fledPlayerHp = state.active[0].hp;
    resolveEndTurn(state, rng, false, result);
    test.assert(result.outcome, Outcome::None,
                "repeated terminal end turn does not replay Fled");
    test.assert(factCount(result), static_cast<uint8_t>(0),
                "repeated terminal end turn emits no duplicate facts");
    test.assert(state.active[0].hp, fledPlayerHp,
                "repeated terminal end turn preserves HP");
    test.assert(state.gather.fleeTurns, static_cast<uint8_t>(0),
                "repeated terminal end turn keeps countdown at zero");

    // Same state plus same RNG gives byte-identical gather/end-turn behavior;
    // this guards against hidden rerolls or duplicate progress writes.
    state = stateFixture();
    state.gatherable = true;
    state.gather.need = 14;
    state.gather.tierRate = 2;
    state.gather.progress = 3;
    state.active[0].hp = 71;
    battle::BattleState duplicate = state;
    ActionResult first;
    ActionResult second;
    script(127);
    resolveAction(state, Side::Player, {ActionKind::Gather, 0}, rng, first);
    script(127);
    resolveAction(duplicate, Side::Player, {ActionKind::Gather, 0}, rng, second);
    test.assert(second.kind, first.kind, "deterministic gather kind");
    test.assert(second.flags, first.flags, "deterministic gather flags");
    test.assert(second.progressBefore, first.progressBefore,
                "deterministic gather before progress");
    test.assert(second.progressAfter, first.progressAfter,
                "deterministic gather after progress");
    test.assert(duplicate.gather.progress, state.gather.progress,
                "deterministic gather mutates the same progress");
    test.assert(duplicate.active[0].hp, state.active[0].hp,
                "deterministic gather preserves the same HP");

    suite.addTest(test);
}

inline void BattleResolveSuite(TestRunner &runner)
{
    TestSuite suite("Battle resolution integration");
    BattleResolveIntegrationTest(suite);
    BattleEscapeIntegrationTest(suite);
    BattleGatherIntegrationTest(suite);
    runner.addTestSuite(suite);
}
