#pragma once

#include "test.hpp"
#include "../src/engine/battle/BattleSetup.hpp"
#include "../src/lib/ReadData.hpp"
#include "../src/player/Player.hpp"

extern Player player;

namespace battle_setup_test_detail {

uint8_t packedMoveByte(uint32_t packed, uint8_t position)
{
    return static_cast<uint8_t>((packed >> (static_cast<uint32_t>(position) * 8UL)) &
                                0xFFUL);
}

const CreatureSeed &originalTrainerSlot(const OpponentSeed &seed, uint8_t slot)
{
    return slot == 0 ? seed.firstCreature :
           slot == 1 ? seed.secondCreature : seed.thirdCreature;
}

void assertAuthoredTrainer(Test &test, const battle::BattleState &state,
                           const CreatureSeed &seed, const char *context)
{
    const battle::Combatant &active = state.active[static_cast<uint8_t>(battle::Side::Opponent)];
    test.assert(active.id, seed.id, context);
    test.assert(active.level, seed.lvl, context);
    for (uint8_t position = 0; position < 4; ++position) {
        const uint8_t expected = packedMoveByte(seed.moves, position);
        test.assert(parseOpponentCreatureSeedMove(seed.moves, position), expected,
                    "decoder matches independent 32-bit byte extraction");
        test.assert(active.moveIds[position], expected, context);
        if (expected == 255) {
            test.assert(active.moves[position].move, static_cast<uint16_t>(0),
                        "empty trainer move has a zero descriptor");
            test.assert(active.moves[position].effect1, Effect::NONE,
                        "empty trainer move has no first effect");
            test.assert(active.moves[position].effect2, Effect::NONE,
                        "empty trainer move has no second effect");
        }
    }
}

void preparePlayer()
{
    player.loadCreature(0, 1);
    player.loadCreature(1, 2);
    player.loadCreature(2, 3);
    player.creatureHPs[0] = 0;
    player.creatureHPs[1] = 70;
    player.creatureHPs[2] = 40;
}

void clearPlayer()
{
    for (uint8_t slot = 0; slot < PARTY_SIZE; ++slot) {
        player.party[slot].id = 0;
        player.creatureHPs[slot] = 0;
    }
}

} // namespace battle_setup_test_detail

void BattleSetupIntegrationTest(TestSuite &suite)
{
    using namespace battle;
    using namespace battle_setup_test_detail;
    Test test(__func__);
    battle::BattleState state;
    preparePlayer();

    beginWild(state, 4, 1, true, 2);
    test.assert(state.partyCount[static_cast<uint8_t>(Side::Player)], 3,
                "wild setup counts player party through terminator");
    test.assert(state.activeSlot[static_cast<uint8_t>(Side::Player)], 1,
                "wild setup chooses lowest live player slot");
    test.assert(state.active[static_cast<uint8_t>(Side::Player)].hp, 70,
                "wild setup imports persistent active HP");
    test.assert(state.partyCount[static_cast<uint8_t>(Side::Opponent)], 1,
                "wild setup has one opponent");
    test.assert(state.trainer, false, "wild setup clears trainer flag");
    test.assert(state.gatherable, true, "wild setup preserves gatherable input");
    test.assert(state.gather.tierRate, 2, "wild setup preserves tier rate");
    test.assert(state.gather.need, 8, "level one gather need clamps to eight");
    test.assert(state.gather.progress, 0, "wild setup clears gather progress");
    test.assert(state.gather.fleeTurns, 6, "wild setup seeds flee countdown");

    beginWild(state, 4, 8, true, 1);
    test.assert(state.gather.need, 12, "level eight gather need uses frozen formula");
    beginWild(state, 4, 12, true, 4);
    test.assert(state.gather.need, 14, "level twelve gather need uses integer division");
    beginWild(state, 4, 40, true, 4);
    test.assert(state.gather.need, 24, "level forty gather need clamps to twenty four");

    beginTrainer(state, 0);
    const OpponentSeed expectedTrainer = readOpponentSeed(0);
    const uint32_t independentBytes = 0x78563412UL;
    for (uint8_t position = 0; position < 4; ++position) {
        test.assert(parseOpponentCreatureSeedMove(independentBytes, position),
                    packedMoveByte(independentBytes, position),
                    "decoder preserves every position with distinct bytes");
    }
    test.assert(state.trainer, true, "trainer setup sets trainer flag");
    test.assert(state.trainerId, static_cast<uint8_t>(0),
                "trainer setup retains authored row identity");
    test.assert(state.gatherable, false, "trainer setup clears gatherable flag");
    test.assert(state.gather.tierRate, 0, "trainer setup clears tier rate");
    test.assert(state.partyCount[static_cast<uint8_t>(Side::Opponent)], 3,
                "trainer setup derives all three seed party slots");
    test.assert(state.active[static_cast<uint8_t>(Side::Opponent)].id, 0,
                "trainer setup accepts species zero seed");
    assertAuthoredTrainer(test, state, originalTrainerSlot(expectedTrainer, 0),
                         "trainer setup retains authored slot zero");
    test.assert(state.bench[static_cast<uint8_t>(Side::Opponent)][0].id, 3,
                "trainer bench keeps original slot one");
    test.assert(state.bench[static_cast<uint8_t>(Side::Opponent)][1].id, 6,
                "trainer bench keeps original slot two");

    state.active[static_cast<uint8_t>(Side::Opponent)].hp = 15;
    state.bench[static_cast<uint8_t>(Side::Opponent)][0].hp = 22;
    ActionResult result;
    test.assert(applySwitch(state, Side::Opponent, 1, true, result), true,
                "ordinary trainer switch succeeds");
    test.assert(state.activeSlot[static_cast<uint8_t>(Side::Opponent)], 1,
                "switch records original active slot");
    test.assert(state.active[static_cast<uint8_t>(Side::Opponent)].id, 3,
                "switch loads incoming species");
    test.assert(state.active[static_cast<uint8_t>(Side::Opponent)].hp, 22,
                "switch restores incoming bench HP");
    assertAuthoredTrainer(test, state, originalTrainerSlot(expectedTrainer, 1),
                         "replacement retains authored slot one");
    test.assert(state.bench[static_cast<uint8_t>(Side::Opponent)][0].id, 0,
                "switch writes outgoing species into rebuilt bench");
    test.assert(state.bench[static_cast<uint8_t>(Side::Opponent)][0].hp, 15,
                "switch writes outgoing HP without healing");
    test.assert(result.kind, ResultKind::Switch, "switch emits separate result");
    test.assert(result.flags, static_cast<uint8_t>(FORCED_SWITCH),
                "forced switch result carries forced flag");
    test.assert(result.index, 3, "switch result names incoming species");
    test.assert(result.speciesBefore[static_cast<uint8_t>(Side::Opponent)], 0,
                "switch result preserves outgoing species");
    test.assert(result.hpBefore[static_cast<uint8_t>(Side::Opponent)], 15,
                "switch result preserves outgoing HP");
    test.assert(result.hpAfter[static_cast<uint8_t>(Side::Opponent)], 22,
                "switch result exposes incoming HP after transition");

    test.assert(applySwitch(state, Side::Opponent, 0, false, result), true,
                "switch back to original slot succeeds");
    test.assert(state.active[static_cast<uint8_t>(Side::Opponent)].id, 0,
                "switch back restores original species");
    test.assert(state.active[static_cast<uint8_t>(Side::Opponent)].hp, 15,
                "switch back round-trips outgoing HP");
    test.assert(state.activeSlot[static_cast<uint8_t>(Side::Opponent)], 0,
                "switch back restores original slot index");
    assertAuthoredTrainer(test, state, originalTrainerSlot(expectedTrainer, 0),
                         "switch back retains authored slot zero");

    state.bench[static_cast<uint8_t>(Side::Opponent)][1].hp = 9;
    test.assert(applySwitch(state, Side::Opponent, 2, true, result), true,
                "third original trainer slot can replace the first");
    test.assert(state.active[static_cast<uint8_t>(Side::Opponent)].hp, 9,
                "third slot preserves depleted bench HP");
    assertAuthoredTrainer(test, state, originalTrainerSlot(expectedTrainer, 2),
                         "replacement retains authored slot two");
    test.assert(applySwitch(state, Side::Opponent, 0, false, result), true,
                "first original slot can return after third");
    test.assert(state.active[static_cast<uint8_t>(Side::Opponent)].hp, 15,
                "first slot preserves depleted HP after third switch");

    const uint8_t activeBeforeRefused =
        state.active[static_cast<uint8_t>(Side::Opponent)].id;
    state.bench[static_cast<uint8_t>(Side::Opponent)][0].hp = 0;
    test.assert(applySwitch(state, Side::Opponent, 1, false, result), false,
                "dead bench switch is refused");
    test.assert(state.active[static_cast<uint8_t>(Side::Opponent)].id,
                activeBeforeRefused, "refused switch leaves active untouched");
    test.assert((result.flags & REFUSED) != 0, true,
                "refused switch result carries refused flag");

    beginWild(state, 4, 8, false, 0);
    const uint8_t wildId = state.active[static_cast<uint8_t>(Side::Opponent)].id;
    test.assert(applySwitch(state, Side::Opponent, 0, false, result), false,
                "one-creature wild refuses active-slot switch");
    test.assert(state.partyCount[static_cast<uint8_t>(Side::Opponent)], 1,
                "one-creature wild has no switchable bench");
    test.assert(state.trainerId, static_cast<uint8_t>(255),
                "wild setup clears trainer row identity");
    test.assert(state.active[static_cast<uint8_t>(Side::Opponent)].id, wildId,
                "one-creature wild ignores empty bench switch");

    beginWild(state, 4, 8, false, 0);
    state.active[static_cast<uint8_t>(Side::Player)].hp = 50;
    test.assert(applySwitch(state, Side::Player, 2, false, result), true,
                "player switch succeeds from original slot");
    test.assert(state.activeSlot[static_cast<uint8_t>(Side::Player)], 2,
                "player switch uses original slot, not bench cursor");
    test.assert(state.active[static_cast<uint8_t>(Side::Player)].hp, 40,
                "player switch restores persistent bench HP");
    test.assert(state.bench[static_cast<uint8_t>(Side::Player)][1].id, 2,
                "player bench order excludes new active slot");
    test.assert(state.bench[static_cast<uint8_t>(Side::Player)][1].hp, 50,
                "player outgoing HP remains mapped by original slot");

    clearPlayer();
    beginWild(state, 4, 8, true, 1);
    test.assert(state.over, true, "entry with no live player is terminal");
    test.assert(state.partyCount[static_cast<uint8_t>(Side::Opponent)], 0,
                "no-live entry performs no opponent setup");

    suite.addTest(test);
}

void BattleSetupSuite(TestRunner &runner)
{
    TestSuite suite("Battle setup integration");
    BattleSetupIntegrationTest(suite);
    runner.addTestSuite(suite);
}
