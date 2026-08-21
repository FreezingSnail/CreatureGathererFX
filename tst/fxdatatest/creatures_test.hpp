#pragma once

#include <avr/pgmspace.h>
#include <string.h>

#include "fxtest.hpp"
#include "generated/creature_data.hpp"
#include "src/lib/ReadData.hpp"
#include "src/opponent/Opponent.hpp"

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

        Opponent encounter;
        loadEncounterOpt(&encounter, id, encounterLevel);

        test.expectEqIdx(encounter.party[0].id, expected.id, F("encounter.id"), id);
        test.expectEqIdx(static_cast<uint8_t>(encounter.party[0].types.getType1()), expected.type1,
                         F("encounter.type1"), id);
        test.expectEqIdx(static_cast<uint8_t>(encounter.party[0].types.getType2()), expected.type2,
                         F("encounter.type2"), id);
        test.expectEqIdx(encounter.party[0].moves[0], expected.move1, F("encounter.move1"), id);
        test.expectEqIdx(encounter.party[0].moves[1], expected.move2, F("encounter.move2"), id);
        test.expectEqIdx(encounter.party[0].moves[2], expected.move3, F("encounter.move3"), id);
        test.expectEqIdx(encounter.party[0].moves[3], expected.move4, F("encounter.move4"), id);

        // The requested level initializes both encounter metadata and creature stats.
        test.expectEqIdx(encounter.levels[0], encounterLevel, F("encounter.level0"), id);
        test.expectEqIdx(encounter.levels[1], 0, F("encounter.level1"), id);
        test.expectEqIdx(encounter.levels[2], 0, F("encounter.level2"), id);
        test.expectEqIdx(encounter.party[0].level, encounterLevel, F("encounter.party0.level"), id);
    }
}
