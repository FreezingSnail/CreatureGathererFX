#pragma once

#include <avr/pgmspace.h>
#include <string.h>

#include "fxtest.hpp"
#include "generated/creature_data.hpp"
#include "src/lib/ReadData.hpp"
#include "src/engine/battle/BattleSetup.hpp"

static_assert(sizeof(Effect) == sizeof(uint8_t), "Effect must be one byte on AVR");
static_assert(sizeof(Move) == 4, "Move must be four bytes on AVR");
static_assert(sizeof(Creature) == 33, "Creature must be 33 bytes on AVR");

void test_creatureData(FxTest &test, const CreatureData_t &actual,
                       const CreatureData_t &expected, uint8_t index) {
    test.expectEqIdx(actual.id, expected.id, F("creature.id"), index);
    test.expectEqIdx(actual.type1, expected.type1, F("creature.type1"), index);
    test.expectEqIdx(actual.type2, expected.type2, F("creature.type2"), index);
    test.expectEqIdx(actual.evoLevel, expected.evoLevel, F("creature.evoLevel"), index);
    test.expectEqIdx(actual.atkSeed, expected.atkSeed, F("creature.atkSeed"), index);
    test.expectEqIdx(actual.defSeed, expected.defSeed, F("creature.defSeed"), index);
    test.expectEqIdx(actual.spcAtkSeed, expected.spcAtkSeed, F("creature.spcAtkSeed"), index);
    test.expectEqIdx(actual.spcDefSeed, expected.spcDefSeed, F("creature.spcDefSeed"), index);
    test.expectEqIdx(actual.hpSeed, expected.hpSeed, F("creature.hpSeed"), index);
    test.expectEqIdx(actual.spdSeed, expected.spdSeed, F("creature.spdSeed"), index);
    test.expectEqIdx(actual.move1, expected.move1, F("creature.move1"), index);
    test.expectEqIdx(actual.move2, expected.move2, F("creature.move2"), index);
    test.expectEqIdx(actual.move3, expected.move3, F("creature.move3"), index);
    test.expectEqIdx(actual.move4, expected.move4, F("creature.move4"), index);
}

void test_creatures(FxTest &test) {
    // Keep one fixture and one FX record on the stack while traversing all records.
    for (uint8_t index = 0; index < creatureFixtureCount; ++index) {
        CreatureData_t expected;
        memcpy_P(&expected, creatureFixtures + index, sizeof(expected));
        const CreatureData_t actual = getCreatureFromStore(index);
        test_creatureData(test, actual, expected, index);
    }

    constexpr uint8_t encounterLevel = 5;
    const uint8_t encounterIds[] = {0, static_cast<uint8_t>(creatureFixtureCount - 1)};
    for (uint8_t fixtureIndex = 0; fixtureIndex < sizeof(encounterIds); ++fixtureIndex) {
        const uint8_t id = encounterIds[fixtureIndex];
        CreatureData_t expected;
        memcpy_P(&expected, creatureFixtures + id, sizeof(expected));

        // The current wild setup stores one active combatant and empty bench
        // slots. Provide a living player so setup can reach the FX load path.
        player.party[0].load(expected);
        player.creatureHPs[0] = player.party[0].statlist.hp;
        for (uint8_t slot = 1; slot < PARTY_SIZE; ++slot)
            player.party[slot].level = 0;
        battle::BattleState encounter;
        battle::beginWild(encounter, id, encounterLevel, true, 1);
        const uint8_t side = static_cast<uint8_t>(battle::Side::Opponent);
        const battle::Combatant &active = encounter.active[side];

        test.expectEqIdx(active.id, expected.id, F("encounter.id"), id);
        test.expectEqIdx(static_cast<uint8_t>(active.types.getType1()), expected.type1,
                         F("encounter.type1"), id);
        test.expectEqIdx(static_cast<uint8_t>(active.types.getType2()), expected.type2,
                         F("encounter.type2"), id);
        test.expectEqIdx(active.moveIds[0], expected.move1, F("encounter.move1"), id);
        test.expectEqIdx(active.moveIds[1], expected.move2, F("encounter.move2"), id);
        test.expectEqIdx(active.moveIds[2], expected.move3, F("encounter.move3"), id);
        test.expectEqIdx(active.moveIds[3], expected.move4, F("encounter.move4"), id);

        // The requested level initializes both encounter metadata and creature stats.
        test.expectEqIdx(active.level, encounterLevel, F("encounter.level0"), id);
        test.expectEqIdx(encounter.bench[side][0].level, 0, F("encounter.level1"), id);
        test.expectEqIdx(encounter.bench[side][1].level, 0, F("encounter.level2"), id);
        test.expectEqIdx(encounter.partyCount[side], 1, F("encounter.partyCount"), id);
    }
}
