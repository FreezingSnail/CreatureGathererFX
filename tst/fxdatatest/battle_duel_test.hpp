#pragma once
// Device proof for the RPS duel kernel (CreatureGathererFX-d73i.1 spike step 1).
// Exercises the PROGMEM win masks, the 14/4/6 scaling, simultaneous KO/draw,
// switch incoming-KO revert and WAIT directly on AVR. Compiled by the fxtest
// device harness; assertions use the FxTest P/F reporter.

#include "fxtest.hpp"
#include "src/engine/battle/DuelKernel.hpp"

// This suite asserts the shipping compact duel profile. Fail loudly if the
// build properties are missing the flags (a sketch-local define would not
// reach separately compiled TUs), rather than silently testing legacy code.
#if !defined(CGFX_DUEL_BATTLE) || !defined(CGFX_BATTLE_COMPACT_RESULT) || \
    !defined(CGFX_BATTLE_NO_FACTS)
#error "test_battle_duel requires the duel compact profile: CGFX_DUEL_BATTLE, CGFX_BATTLE_COMPACT_RESULT, CGFX_BATTLE_NO_FACTS"
#endif
#if !defined(FX_READ_COUNTER)
#error "test_battle_duel requires -DFX_READ_COUNTER so metadata-read assertions are non-vacuous"
#endif

#if defined(CGFX_DUEL_BATTLE) && defined(__AVR__)
// Canonical AVR route: load the packed 25-cell chart and check the same
// relations through the RAM lookup. The buffer is defined in BattleSession.cpp.
#include "src/fxdata.h"
#include "src/lib/FxRead.hpp"
#endif



namespace battle_duel_device_detail {

uint32_t rngState = 0x2468ace0u;

uint8_t duelRoll(uint8_t bound)
{
    uint32_t x = rngState;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    rngState = x;
    return bound == 0 ? 0 : static_cast<uint8_t>((x >> 16) % bound);
}

battle::Rng makeRng()
{
    battle::Rng rng;
    rng.next = &duelRoll;
    return rng;
}

const uint8_t Spirit = static_cast<uint8_t>(duel::DType::Spirit);
const uint8_t Water = static_cast<uint8_t>(duel::DType::Water);
const uint8_t Earth = static_cast<uint8_t>(duel::DType::Earth);
const uint8_t Fire = static_cast<uint8_t>(duel::DType::Fire);
const uint8_t Plant = static_cast<uint8_t>(duel::DType::Plant);

// mock/matrices.js id "5" chart: 1 neutral, 2 win, 0 lose.
const uint8_t kChart[25] = {
    1, 0, 0, 2, 0,
    2, 1, 2, 2, 0,
    2, 0, 1, 2, 2,
    0, 0, 0, 1, 2,
    2, 2, 0, 0, 1,
};

duel::Member makeMember(uint8_t id, uint8_t type, const uint8_t *options,
                        uint8_t optionCount, uint8_t hp)
{
    duel::Member m = {};
    m.id = id;
    m.typeCount = 1;
    m.types[0] = type;
    m.hp = m.maxHp = hp;
    m.optionCount = optionCount;
    for (uint8_t i = 0; i < optionCount; ++i) m.options[i] = options[i];
    return m;
}

} // namespace battle_duel_device_detail

void battle_duel_device_test(FxTest &test)
{
    using namespace battle_duel_device_detail;

#if defined(CGFX_DUEL_BATTLE) && defined(__AVR__)
    FxRead::bytes(duelChartFixture, duel::duelChart, sizeof(duel::duelChart));
#endif

    // All 25 relations through the canonical relation source for this target.
    for (uint8_t a = 0; a < duel::TYPE_COUNT; ++a) {
        for (uint8_t d = 0; d < duel::TYPE_COUNT; ++d) {
            const uint8_t expected = kChart[a * duel::TYPE_COUNT + d] == 2
                ? duel::REL_WIN
                : kChart[a * duel::TYPE_COUNT + d] == 0 ? duel::REL_LOSE
                                                        : duel::REL_TRADE;
            test.expectEq(duel::relation(a, d), expected, F("relation"));
        }
    }

    // Pinned integer amounts 21/7/14, 6/2/4, 9/3/6.
    test.expectEq(duel::scaleDamage(14, duel::MOD_BONUS), 21, F("win1.5"));
    test.expectEq(duel::scaleDamage(14, duel::MOD_HALF), 7, F("win0.5"));
    test.expectEq(duel::scaleDamage(4, duel::MOD_BONUS), 6, F("chip1.5"));
    test.expectEq(duel::scaleDamage(4, duel::MOD_HALF), 2, F("chip0.5"));
    test.expectEq(duel::scaleDamage(6, duel::MOD_BONUS), 9, F("trade1.5"));
    test.expectEq(duel::scaleDamage(6, duel::MOD_HALF), 3, F("trade0.5"));

    // Advantage-first dual rule.
    const uint8_t plantWater[2] = {Plant, Water};
    test.expectEq(duel::creatureModifier(Fire, plantWater, 2), duel::MOD_BONUS,
                  F("dualAdv"));
    const uint8_t earthFire[2] = {Earth, Fire};
    test.expectEq(duel::creatureModifier(Plant, earthFire, 2), duel::MOD_HALF,
                  F("dualWeak"));

    // Simultaneous KO -> draw.
    {
        const uint8_t playerOpts[1] = {Spirit};
        const uint8_t foeOpts[1] = {Water};
        duel::State state = {};
        state.sides[0].count = 1;
        state.sides[0].active = 0;
        state.sides[0].members[0] = makeMember(1, Spirit, playerOpts, 1, 4);
        state.sides[1].count = 1;
        state.sides[1].active = 0;
        state.sides[1].members[0] = makeMember(2, Spirit, foeOpts, 1, 4);
        const duel::Action actions[2] = {{duel::ActionKind::Attack, 0},
                                         {duel::ActionKind::Attack, 0}};
        battle::Rng rng = makeRng();
        duel::RoundResult out = {};
        duel::resolveRound(state, actions, rng, out);
        test.expectEq(state.sides[0].members[0].hp, 0, F("koP"));
        test.expectEq(state.sides[1].members[0].hp, 0, F("koF"));
        test.expectEq(static_cast<uint8_t>(state.outcome),
                      static_cast<uint8_t>(duel::Outcome::Draw), F("draw"));
    }

    // Switch incoming KO reverts to the outgoing (living) member.
    {
        const uint8_t playerOpts[1] = {Fire};
        duel::State state = {};
        state.sides[0].count = 1;
        state.sides[0].active = 0;
        state.sides[0].members[0] = makeMember(1, Plant, playerOpts, 1, 100);
        state.sides[1].count = 2;
        state.sides[1].active = 0;
        const uint8_t foeOpts[1] = {Spirit};
        state.sides[1].members[0] = makeMember(2, Spirit, foeOpts, 1, 100);
        state.sides[1].members[1] = makeMember(3, Plant, foeOpts, 1, 10);
        const duel::Action actions[2] = {{duel::ActionKind::Attack, 0},
                                         {duel::ActionKind::Switch, 1}};
        battle::Rng rng = makeRng();
        duel::RoundResult out = {};
        duel::resolveRound(state, actions, rng, out);
        test.expectEq(state.sides[1].members[1].hp, 0, F("inKO"));
        test.expectEq(state.sides[1].active, 0, F("revert"));
    }

    // WAIT receives the opponent's default win.
    {
        duel::State state = {};
        state.sides[0].count = 1;
        state.sides[0].active = 0;
        const uint8_t noOpts[0] = {};
        state.sides[0].members[0] = makeMember(1, Fire, noOpts, 0, 100);
        state.sides[1].count = 1;
        state.sides[1].active = 0;
        const uint8_t foeOpts[1] = {Spirit};
        state.sides[1].members[0] = makeMember(2, Spirit, foeOpts, 1, 100);
        const duel::Action actions[2] = {{duel::ActionKind::Wait, duel::NO_SLOT},
                                         {duel::ActionKind::Attack, 0}};
        battle::Rng rng = makeRng();
        duel::RoundResult out = {};
        duel::resolveRound(state, actions, rng, out);
        // spirit beats fire -> x1.5 -> 21 on the waiting side.
        test.expectEq(out.damage[0], 21, F("waitHit"));
        test.expectEq(out.damage[1], 0, F("waitDeal"));
    }
}
