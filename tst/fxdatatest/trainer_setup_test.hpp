#pragma once

#include "fxtest.hpp"
#include "src/engine/battle/BattleSetup.hpp"
#include "src/lib/FxReadCounter.hpp"
#include "src/lib/ReadData.hpp"
#include "src/player/Player.hpp"

inline uint8_t trainerExpectedMove(uint32_t packed, uint8_t position)
{
    return static_cast<uint8_t>((packed >> (static_cast<uint32_t>(position) * 8UL)) &
                                0xFFUL);
}

inline const CreatureSeed &trainerOriginalSeed(const OpponentSeed &row, uint8_t slot)
{
    return slot == 0 ? row.firstCreature :
           slot == 1 ? row.secondCreature : row.thirdCreature;
}

inline void checkTrainerActive(FxTest &test, const battle::BattleState &state,
                               const CreatureSeed &seed)
{
    const battle::Combatant &active = state.active[static_cast<uint8_t>(battle::Side::Opponent)];
    test.expectEq(active.id, seed.id, F("authored trainer species"));
    test.expectEq(active.level, seed.lvl, F("authored trainer level"));
    for (uint8_t position = 0; position < 4; ++position) {
        const uint8_t expected = trainerExpectedMove(seed.moves, position);
        test.expectEq(parseOpponentCreatureSeedMove(seed.moves, position), expected,
                      F("packed trainer byte decoder"));
        test.expectEq(active.moveIds[position], expected, F("authored trainer move"));
        if (expected == 255) {
            test.expectEq(active.moves[position].move, 0, F("empty move descriptor"));
            test.expectEq(static_cast<uint8_t>(active.moves[position].effect1),
                          static_cast<uint8_t>(Effect::NONE), F("empty move effect1"));
            test.expectEq(static_cast<uint8_t>(active.moves[position].effect2),
                          static_cast<uint8_t>(Effect::NONE), F("empty move effect2"));
        }
    }
}

inline void test_trainer_setup(FxTest &test)
{
    constexpr uint8_t trainerId = 0;
    const OpponentSeed row = readOpponentSeed(trainerId);
    test.expectEq(row.firstCreature.lvl != 0, true, F("trainer first slot populated"));
    test.expectEq(row.secondCreature.lvl != 0, true, F("trainer second slot populated"));
    test.expectEq(row.thirdCreature.lvl != 0, true, F("trainer third slot populated"));
    test.expectEq(row.firstCreature.id != row.secondCreature.id &&
                  row.secondCreature.id != row.thirdCreature.id &&
                  row.firstCreature.id != row.thirdCreature.id, true,
                  F("trainer fixture has distinct species"));

    player.basic();
    battle::BattleState state;
    battle::ActionResult result;
    FxReadCounter::resetFrame();
    battle::beginTrainer(state, trainerId);
    test.expectEq(FxReadCounter::count(), 8, F("trainer start reads two row records and six assets"));
    test.expectEq(FxReadCounter::framePassed(), true, F("trainer start transition budget"));
    test.expectEq(state.trainerId, trainerId, F("trainer identity retained"));
    test.expectEq(state.partyCount[1], 3, F("trainer has three party slots"));
    checkTrainerActive(test, state, trainerOriginalSeed(row, 0));

    state.active[1].hp = 11;
    state.bench[1][0].hp = 23;
    FxReadCounter::resetFrame();
    test.expectEq(battle::applySwitch(state, battle::Side::Opponent, 1, true, result),
                  true, F("trainer slot one replacement"));
    test.expectEq(FxReadCounter::count(), 4, F("trainer replacement reads row and two assets"));
    test.expectEq(FxReadCounter::framePassed(), true, F("trainer replacement transition budget"));
    test.expectEq(state.active[1].hp, 23, F("trainer slot one retained HP"));
    checkTrainerActive(test, state, trainerOriginalSeed(row, 1));

    state.bench[1][1].hp = 7;
    FxReadCounter::resetFrame();
    test.expectEq(battle::applySwitch(state, battle::Side::Opponent, 2, true, result),
                  true, F("trainer slot two replacement"));
    test.expectEq(FxReadCounter::count(), 4, F("third slot transition read budget"));
    test.expectEq(FxReadCounter::framePassed(), true, F("third slot transition exact"));
    test.expectEq(state.active[1].hp, 7, F("trainer third slot retained HP"));
    checkTrainerActive(test, state, trainerOriginalSeed(row, 2));

    FxReadCounter::resetFrame();
    test.expectEq(battle::applySwitch(state, battle::Side::Opponent, 0, false, result),
                  true, F("trainer original slot returns"));
    test.expectEq(FxReadCounter::count(), 4, F("return transition read budget"));
    test.expectEq(state.active[1].hp, 11, F("original slot HP stayed depleted"));
    checkTrainerActive(test, state, trainerOriginalSeed(row, 0));
}
