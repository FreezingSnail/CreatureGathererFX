#pragma once
// Permanent host proof for the RPS duel kernel (CreatureGathererFX-d73i.1).
// Compiled only in duel host runs: `make test CPPFLAGS='-I. -DCGFX_DUEL_BATTLE'`.

#include "test.hpp"
#include "../src/engine/battle/DuelKernel.hpp"

namespace duel_kernel_test_detail {

uint32_t rngState = 0x12345678u;

uint8_t seededRoll(uint8_t bound)
{
    // xorshift32; high bits feed the bounded draw so every residue is reachable.
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
    rng.next = &seededRoll;
    return rng;
}

duel::Member makeMember(uint8_t id, const uint8_t *types, uint8_t typeCount,
                        const uint8_t *options, uint8_t optionCount, uint8_t hp)
{
    duel::Member m = {};
    m.id = id;
    m.typeCount = typeCount;
    m.types[0] = types[0];
    m.types[1] = typeCount > 1 ? types[1] : static_cast<uint8_t>(0);
    m.hp = hp;
    m.maxHp = hp;
    m.optionCount = optionCount;
    for (uint8_t i = 0; i < optionCount; ++i) m.options[i] = options[i];
    return m;
}

// Single-member sides by default; callers add bench where needed. Member HP
// is taken verbatim from the arguments so KO states are expressible.
duel::State makeState(duel::Member player, duel::Member opponent)
{
    duel::State state = {};
    state.sides[0].count = 1;
    state.sides[0].active = 0;
    state.sides[0].members[0] = player;
    state.sides[1].count = 1;
    state.sides[1].active = 0;
    state.sides[1].members[0] = opponent;
    state.outcome = duel::Outcome::None;
    return state;
}

const uint8_t Spirit = static_cast<uint8_t>(duel::DType::Spirit);
const uint8_t Water = static_cast<uint8_t>(duel::DType::Water);
const uint8_t Earth = static_cast<uint8_t>(duel::DType::Earth);
const uint8_t Fire = static_cast<uint8_t>(duel::DType::Fire);
const uint8_t Plant = static_cast<uint8_t>(duel::DType::Plant);

// mock/matrices.js id "5" chart flattened: [attacker*5 + defender].
const uint8_t kChart[25] = {
    1, 0, 0, 2, 0, // spirit
    2, 1, 2, 2, 0, // water
    2, 0, 1, 2, 2, // earth
    0, 0, 0, 1, 2, // fire
    2, 2, 0, 0, 1, // plant
};

} // namespace duel_kernel_test_detail

void DuelRelationSuite(TestSuite &suite)
{
    using namespace duel_kernel_test_detail;
    Test test(__func__);

    // All 25 pick relations match the pinned 5-type chart (10 win edges).
    for (uint8_t a = 0; a < duel::TYPE_COUNT; ++a) {
        for (uint8_t d = 0; d < duel::TYPE_COUNT; ++d) {
            const uint8_t expected =
                kChart[a * duel::TYPE_COUNT + d] == 2 ? duel::REL_WIN
                : kChart[a * duel::TYPE_COUNT + d] == 0 ? duel::REL_LOSE
                : duel::REL_TRADE;
            test.assert(duel::relation(a, d), expected,
                        std::string("relation ") + std::to_string(a) + "->" +
                            std::to_string(d));
        }
    }
    suite.addTest(test);
}

void DuelFoldAndDamageSuite(TestSuite &suite)
{
    using namespace duel_kernel_test_detail;
    Test test(__func__);

    // Fold map from the authored 8 types: lightning->fire, wind->water,
    // elder->spirit; the rest keep their identity.
    test.assert(duel::foldType(0), Spirit, "spirit stays");
    test.assert(duel::foldType(1), Water, "water stays");
    test.assert(duel::foldType(2), Water, "wind folds to water");
    test.assert(duel::foldType(3), Earth, "earth stays");
    test.assert(duel::foldType(4), Fire, "fire stays");
    test.assert(duel::foldType(5), Fire, "lightning folds to fire");
    test.assert(duel::foldType(6), Plant, "plant stays");
    test.assert(duel::foldType(7), Spirit, "elder folds to spirit");

    // Pinned integer amounts: 21/7/14, 6/2/4, 9/3/6.
    test.assert(duel::scaleDamage(14, duel::MOD_BONUS), static_cast<uint8_t>(21), "win x1.5");
    test.assert(duel::scaleDamage(14, duel::MOD_SAME), static_cast<uint8_t>(14), "win x1");
    test.assert(duel::scaleDamage(14, duel::MOD_HALF), static_cast<uint8_t>(7), "win x0.5");
    test.assert(duel::scaleDamage(4, duel::MOD_BONUS), static_cast<uint8_t>(6), "chip x1.5");
    test.assert(duel::scaleDamage(4, duel::MOD_SAME), static_cast<uint8_t>(4), "chip x1");
    test.assert(duel::scaleDamage(4, duel::MOD_HALF), static_cast<uint8_t>(2), "chip x0.5");
    test.assert(duel::scaleDamage(6, duel::MOD_BONUS), static_cast<uint8_t>(9), "trade x1.5");
    test.assert(duel::scaleDamage(6, duel::MOD_SAME), static_cast<uint8_t>(6), "trade x1");
    test.assert(duel::scaleDamage(6, duel::MOD_HALF), static_cast<uint8_t>(3), "trade x0.5");

    // Advantage-first dual rule: any win -> x1.5, else any loss -> x0.5.
    const uint8_t plantWater[2] = {Plant, Water};
    test.assert(duel::creatureModifier(Fire, plantWater, 2), duel::MOD_BONUS,
                "dual conflict: a beaten type wins the advantage");
    const uint8_t earthFire[2] = {Earth, Fire};
    test.assert(duel::creatureModifier(Plant, earthFire, 2), duel::MOD_HALF,
                "dual losses give x0.5 with no multiplied weakness");
    const uint8_t spiritPlant[2] = {Spirit, Plant};
    test.assert(duel::creatureModifier(Earth, spiritPlant, 2), duel::MOD_BONUS,
                "earth beats both defender types");
    const uint8_t fireWater[2] = {Fire, Water};
    test.assert(duel::creatureModifier(Spirit, fireWater, 2), duel::MOD_BONUS,
                "spirit's win beats its loss under advantage-first");
    suite.addTest(test);
}

void DuelRoundSuite(TestSuite &suite)
{
    using namespace duel_kernel_test_detail;
    Test test(__func__);

    // Win/chip with no creature modifier: player spirit beats foe fire.
    {
        const uint8_t playerTypes[1] = {Fire};
        const uint8_t foeTypes[1] = {Spirit};
        const uint8_t playerOpts[1] = {Spirit};
        const uint8_t foeOpts[1] = {Fire};
        duel::State state = makeState(
            makeMember(1, playerTypes, 1, playerOpts, 1, 100),
            makeMember(2, foeTypes, 1, foeOpts, 1, 100));
        const duel::Action actions[2] = {{duel::ActionKind::Attack, 0},
                                         {duel::ActionKind::Attack, 0}};
        battle::Rng rng = makeRng();
        duel::RoundResult out = {};
        duel::resolveRound(state, actions, rng, out);
        test.assert(out.relation, duel::REL_WIN, "spirit beats fire");
        test.assert(out.damage[1], static_cast<uint8_t>(14), "win deals 14");
        test.assert(out.damage[0], static_cast<uint8_t>(4), "chip deals 4");
        test.assert(state.sides[1].members[0].hp, static_cast<uint8_t>(86), "foe hp");
        test.assert(state.sides[0].members[0].hp, static_cast<uint8_t>(96), "player hp");
    }

    // Bonus/weak scaling on a winning hit: spirit beats fire and also beats the
    // foe's fire creature type, so the 14 base becomes 21.
    {
        const uint8_t playerTypes[1] = {Fire};
        const uint8_t foeTypes[1] = {Fire};
        const uint8_t playerOpts[1] = {Spirit};
        const uint8_t foeOpts[1] = {Fire};
        duel::State state = makeState(
            makeMember(1, playerTypes, 1, playerOpts, 1, 100),
            makeMember(2, foeTypes, 1, foeOpts, 1, 100));
        const duel::Action actions[2] = {{duel::ActionKind::Attack, 0},
                                         {duel::ActionKind::Attack, 0}};
        battle::Rng rng = makeRng();
        duel::RoundResult out = {};
        duel::resolveRound(state, actions, rng, out);
        test.assert(out.relation, duel::REL_WIN, "spirit beats fire move relation");
        test.assert(out.damage[1], static_cast<uint8_t>(21), "bonus win deals 21");
        test.assert(out.damage[0], static_cast<uint8_t>(4), "chip deals 4");
    }

    // Trade: mirrored same type deals 6/6.
    {
        const uint8_t types[1] = {Spirit};
        const uint8_t opts[1] = {Spirit};
        duel::State state = makeState(makeMember(1, types, 1, opts, 1, 100),
                                      makeMember(2, types, 1, opts, 1, 100));
        const duel::Action actions[2] = {{duel::ActionKind::Attack, 0},
                                         {duel::ActionKind::Attack, 0}};
        battle::Rng rng = makeRng();
        duel::RoundResult out = {};
        duel::resolveRound(state, actions, rng, out);
        test.assert(out.relation, duel::REL_TRADE, "mirror is neutral");
        test.assert(out.damage[0], static_cast<uint8_t>(6), "trade 6 player");
        test.assert(out.damage[1], static_cast<uint8_t>(6), "trade 6 foe");
    }

    // Simultaneous KO -> draw (both sides' last members faint in one round).
    {
        const uint8_t types[1] = {Spirit};
        const uint8_t playerOpts[1] = {Spirit};
        const uint8_t foeOpts[1] = {Water};
        duel::State state = makeState(
            makeMember(1, types, 1, playerOpts, 1, 4),
            makeMember(2, types, 1, foeOpts, 1, 4));
        const duel::Action actions[2] = {{duel::ActionKind::Attack, 0},
                                         {duel::ActionKind::Attack, 0}};
        battle::Rng rng = makeRng();
        duel::RoundResult out = {};
        // water beats spirit: water wins 21 vs chip 4, both fall in one round.
        duel::resolveRound(state, actions, rng, out);
        test.assert(state.sides[0].members[0].hp, static_cast<uint8_t>(0), "player KO");
        test.assert(state.sides[1].members[0].hp, static_cast<uint8_t>(0), "foe KO");
        test.assert(state.outcome, duel::Outcome::Draw, "simultaneous KO draws");
    }

    // Ordinary KO promotes the first living original slot.
    {
        const uint8_t playerTypes[1] = {Fire};
        const uint8_t foeTypes[1] = {Spirit};
        const uint8_t playerOpts[1] = {Spirit}; // spirit beats fire
        const uint8_t foeOpts[1] = {Fire};
        duel::State state = makeState(
            makeMember(1, playerTypes, 1, playerOpts, 1, 100),
            makeMember(2, foeTypes, 1, foeOpts, 1, 10));
        const uint8_t benchTypes[1] = {Spirit};
        const uint8_t benchOpts[1] = {Fire};
        state.sides[1].members[1] = makeMember(3, benchTypes, 1, benchOpts, 1, 50);
        state.sides[1].count = 2;
        const duel::Action actions[2] = {{duel::ActionKind::Attack, 0},
                                         {duel::ActionKind::Attack, 0}};
        battle::Rng rng = makeRng();
        duel::RoundResult out = {};
        duel::resolveRound(state, actions, rng, out);
        test.assert(state.sides[1].members[0].hp, static_cast<uint8_t>(0), "active KO");
        test.assert(state.sides[1].active, static_cast<uint8_t>(1), "promote slot 1");
        test.assert(state.outcome, duel::Outcome::None, "battle continues");
    }

    suite.addTest(test);
}

void DuelSwitchSuite(TestSuite &suite)
{
    using namespace duel_kernel_test_detail;
    Test test(__func__);

    // Default win: the non-switching side hits the incoming creature's type.
    {
        const uint8_t playerTypes[1] = {Fire};
        const uint8_t foeTypes[1] = {Spirit}; // incoming type
        const uint8_t playerOpts[1] = {Spirit};
        const uint8_t foeOpts[1] = {Fire};
        duel::State state = makeState(
            makeMember(1, playerTypes, 1, playerOpts, 1, 100),
            makeMember(2, foeTypes, 1, foeOpts, 1, 100));
        const uint8_t benchTypes[1] = {Spirit};
        const uint8_t benchOpts[1] = {Water};
        state.sides[1].members[1] = makeMember(3, benchTypes, 1, benchOpts, 1, 100);
        state.sides[1].count = 2;
        const duel::Action actions[2] = {{duel::ActionKind::Attack, 0},
                                         {duel::ActionKind::Switch, 1}};
        battle::Rng rng = makeRng();
        duel::RoundResult out = {};
        duel::resolveRound(state, actions, rng, out);
        test.assert(state.sides[1].active, static_cast<uint8_t>(1), "foe switched");
        test.assert(out.damage[0], static_cast<uint8_t>(0), "switcher deals no damage");
        // spirit (player) beats spirit? no: spirit vs spirit neutral -> 14.
        test.assert(out.damage[1], static_cast<uint8_t>(14), "default win 14");
    }

    // Both switch: zero damage, both active change.
    {
        const uint8_t types[1] = {Spirit};
        const uint8_t opts[1] = {Spirit};
        duel::State state = makeState(makeMember(1, types, 1, opts, 1, 100),
                                      makeMember(2, types, 1, opts, 1, 100));
        state.sides[0].members[1] = makeMember(3, types, 1, opts, 1, 100);
        state.sides[0].count = 2;
        state.sides[1].members[1] = makeMember(4, types, 1, opts, 1, 100);
        state.sides[1].count = 2;
        const duel::Action actions[2] = {{duel::ActionKind::Switch, 1},
                                         {duel::ActionKind::Switch, 1}};
        battle::Rng rng = makeRng();
        duel::RoundResult out = {};
        duel::resolveRound(state, actions, rng, out);
        test.assert(out.damage[0], static_cast<uint8_t>(0), "no damage side 0");
        test.assert(out.damage[1], static_cast<uint8_t>(0), "no damage side 1");
        test.assert(state.sides[0].active, static_cast<uint8_t>(1), "player switched");
        test.assert(state.sides[1].active, static_cast<uint8_t>(1), "foe switched");
    }

    // Incoming KO reverts to the outgoing member when it is still living.
    {
        const uint8_t playerTypes[1] = {Plant};
        const uint8_t foeTypes[1] = {Spirit};
        const uint8_t playerOpts[1] = {Fire}; // fire beats plant -> x1.5 -> 21
        const uint8_t foeOpts[1] = {Spirit};
        duel::State state = makeState(
            makeMember(1, playerTypes, 1, playerOpts, 1, 100),
            makeMember(2, foeTypes, 1, foeOpts, 1, 100));
        // Incoming is a fresh creature with only 10 HP.
        const uint8_t incomingTypes[1] = {Plant};
        const uint8_t incomingOpts[1] = {Spirit};
        state.sides[1].members[1] = makeMember(3, incomingTypes, 1, incomingOpts, 1, 10);
        state.sides[1].count = 2;
        const duel::Action actions[2] = {{duel::ActionKind::Attack, 0},
                                         {duel::ActionKind::Switch, 1}};
        battle::Rng rng = makeRng();
        duel::RoundResult out = {};
        duel::resolveRound(state, actions, rng, out);
        test.assert(state.sides[1].members[1].hp, static_cast<uint8_t>(0),
                    "incoming KO'd");
        test.assert(state.sides[1].active, static_cast<uint8_t>(0),
                    "incoming KO reverts to the outgoing member");
    }

    suite.addTest(test);
}

void DuelWaitAndEdgeSuite(TestSuite &suite)
{
    using namespace duel_kernel_test_detail;
    Test test(__func__);

    // WAIT commits no attack and receives the opponent's default win.
    {
        const uint8_t playerTypes[1] = {Fire};
        const uint8_t foeTypes[1] = {Spirit};
        const uint8_t playerOpts[0] = {};
        const uint8_t foeOpts[1] = {Spirit};
        duel::State state = makeState(
            makeMember(1, playerTypes, 1, playerOpts, 0, 100),
            makeMember(2, foeTypes, 1, foeOpts, 1, 100));
        const duel::Action actions[2] = {{duel::ActionKind::Wait, duel::NO_SLOT},
                                         {duel::ActionKind::Attack, 0}};
        battle::Rng rng = makeRng();
        duel::RoundResult out = {};
        duel::resolveRound(state, actions, rng, out);
        test.assert(out.damage[1], static_cast<uint8_t>(0), "attacker takes none");
        // spirit beats fire -> x1.5 on 14 = 21, applied to the waiting side.
        test.assert(out.damage[0], static_cast<uint8_t>(21), "waiter takes 21");
    }

    // A kit with no usable attack yields WAIT and never deadlocks.
    {
        const uint8_t types[1] = {Spirit};
        const uint8_t noOpts[0] = {};
        duel::State state = makeState(makeMember(1, types, 1, noOpts, 0, 100),
                                      makeMember(2, types, 1, noOpts, 0, 100));
        battle::Rng rng = makeRng();
        const duel::Action act = duel::chooseAction(state, duel::Persona::Reader,
                                                    50, rng);
        test.assert(act.kind, duel::ActionKind::Wait, "empty kit waits");
    }

    // syncActive promotes the first living slot; a side loses only when all
    // members are gone.
    {
        const uint8_t types[1] = {Spirit};
        const uint8_t opts[1] = {Spirit};
        duel::State state = makeState(makeMember(1, types, 1, opts, 1, 100),
                                      makeMember(2, types, 1, opts, 1, 0));
        state.sides[1].members[1] = makeMember(3, types, 1, opts, 1, 0);
        state.sides[1].count = 2;
        test.assert(duel::sideAlive(state, 1), false, "exhausted side is dead");
        test.assert(duel::firstLivingSlot(state, 1), duel::NO_SLOT, "no living slot");
    }

    suite.addTest(test);
}

void DuelPersonaSuite(TestSuite &suite)
{
    using namespace duel_kernel_test_detail;
    Test test(__func__);

    // History caps at eight entries and keeps the newest.
    duel::State state = {};
    for (uint8_t i = 0; i < 10; ++i) duel::recordPlayerHistory(state, i % 5);
    test.assert(state.historyCount, static_cast<uint8_t>(duel::HISTORY_CAP),
                "history capped at 8");
    test.assert(state.history[duel::HISTORY_CAP - 1], static_cast<uint8_t>(4),
                "newest entry retained (9 % 5)");

    // Reader predicts the last revealed type.
    duel::State reader = {};
    duel::recordPlayerHistory(reader, Water);
    duel::recordPlayerHistory(reader, Earth);
    battle::Rng rng = makeRng();
    test.assert(duel::predictPlayerType(reader.history, reader.historyCount,
                                        duel::Persona::Reader, rng), Earth,
                "reader reads the last type");

    // Frequency predicts the modal type.
    duel::State freq = {};
    duel::recordPlayerHistory(freq, Water);
    duel::recordPlayerHistory(freq, Water);
    duel::recordPlayerHistory(freq, Fire);
    rng = makeRng();
    test.assert(duel::predictPlayerType(freq.history, freq.historyCount,
                                        duel::Persona::Frequency, rng), Water,
                "frequency reads the modal type");

    // Anti-repeat discounts the newest entry: two old water beats one new fire.
    duel::State anti = {};
    duel::recordPlayerHistory(anti, Water);
    duel::recordPlayerHistory(anti, Water);
    duel::recordPlayerHistory(anti, Fire);
    rng = makeRng();
    test.assert(duel::predictPlayerType(anti.history, anti.historyCount,
                                        duel::Persona::AntiRepeat, rng), Water,
                "anti-repeat discounts the newest pick");

    // Random never reads.
    duel::State anyRead = {};
    anyRead.sides[1].count = 1;
    anyRead.sides[1].active = 0;
    const uint8_t types[1] = {Spirit};
    const uint8_t opts[1] = {Spirit};
    anyRead.sides[1].members[0] = makeMember(2, types, 1, opts, 1, 100);
    anyRead.sides[0].count = 1;
    anyRead.sides[0].active = 0;
    anyRead.sides[0].members[0] = makeMember(1, types, 1, opts, 1, 100);
    duel::recordPlayerHistory(anyRead, Water);
    bool alwaysAttack = true;
    for (uint8_t i = 0; i < 16; ++i) {
        rng = makeRng();
        const duel::Action act = duel::chooseAction(anyRead, duel::Persona::Random,
                                                    100, rng);
        if (act.kind != duel::ActionKind::Attack) alwaysAttack = false;
    }
    test.assert(alwaysAttack, true, "random persona never reads (attacks only)");

    // A reader that reads counters the guessed type with the beating option.
    // Plant (index 0) loses to Fire, water (index 1) beats it: exactly one
    // counter exists, so the pick is deterministic.
    duel::State counter = {};
    const uint8_t foeTypes[1] = {Spirit};
    const uint8_t foeOpts[2] = {Plant, Water};
    counter.sides[1].count = 1;
    counter.sides[1].active = 0;
    counter.sides[1].members[0] = makeMember(2, foeTypes, 1, foeOpts, 2, 100);
    counter.sides[0].count = 1;
    counter.sides[0].active = 0;
    const uint8_t playerTypes[1] = {Plant};
    const uint8_t playerOpts[1] = {Plant};
    counter.sides[0].members[0] = makeMember(1, playerTypes, 1, playerOpts, 1, 100);
    duel::recordPlayerHistory(counter, Fire); // reader guesses fire
    rng = makeRng();
    const duel::Action act = duel::chooseAction(counter, duel::Persona::Reader,
                                                100, rng);
    test.assert(act.kind, duel::ActionKind::Attack, "countering action attacks");
    test.assert(act.index, static_cast<uint8_t>(1),
                "reader counters with the beating option");

    // A failed read falls back uniformly across ALL attack slots, whereas a
    // no-read pick prefers the best creature-matchup tier. Foe options are
    // {Plant, Spirit} against a fire player creature: Spirit is the only
    // best-tier slot, and neither beats the guessed Earth, so the branches
    // must be distinguishable.
    duel::State branch = {};
    const uint8_t branchFoeTypes[1] = {Spirit};
    const uint8_t branchOpts[2] = {Plant, Spirit};
    branch.sides[1].count = 1;
    branch.sides[1].active = 0;
    branch.sides[1].members[0] = makeMember(2, branchFoeTypes, 1, branchOpts, 2, 100);
    branch.sides[0].count = 1;
    branch.sides[0].active = 0;
    const uint8_t branchPlayerTypes[1] = {Fire};
    const uint8_t branchPlayerOpts[1] = {Spirit};
    branch.sides[0].members[0] =
        makeMember(1, branchPlayerTypes, 1, branchPlayerOpts, 1, 100);

    // No read (Random never reads): every pick is the best-tier Spirit slot.
    bool noReadOnlyBest = true;
    for (uint8_t i = 0; i < 32; ++i) {
        rng = makeRng();
        const duel::Action a = duel::chooseAction(branch, duel::Persona::Random,
                                                  0, rng);
        if (a.kind != duel::ActionKind::Attack || a.index != 1) noReadOnlyBest = false;
    }
    test.assert(noReadOnlyBest, true, "no-read prefers the matchup-best slot");

    // Fired read with no counter: uniform across slots, so the non-best Plant
    // slot appears across seeds (it can never appear in the no-read branch).
    duel::State readBranch = branch;
    duel::recordPlayerHistory(readBranch, Earth); // no foe option beats Earth
    bool failedReadSawNonBest = false;
    for (uint8_t i = 0; i < 32; ++i) {
        rng = makeRng();
        const duel::Action a = duel::chooseAction(readBranch, duel::Persona::Reader,
                                                  100, rng);
        if (a.kind == duel::ActionKind::Attack && a.index == 0)
            failedReadSawNonBest = true;
    }
    test.assert(failedReadSawNonBest, true,
                "failed read falls back uniformly across all slots");

    suite.addTest(test);
}

void DuelKernelSuite(TestRunner &runner)
{
    TestSuite suite("Duel kernel");
    DuelRelationSuite(suite);
    DuelFoldAndDamageSuite(suite);
    DuelRoundSuite(suite);
    DuelSwitchSuite(suite);
    DuelWaitAndEdgeSuite(suite);
    DuelPersonaSuite(suite);
    runner.addTestSuite(suite);
}
