#pragma once

#include <avr/pgmspace.h>
#include <string.h>

#include "fxtest.hpp"
#include "generated/creature_data.hpp"
#include "generated/evolution_data.hpp"
#include "src/fxdata.h"
#include "src/lib/ReadData.hpp"
#include "src/engine/battle/BattleSetup.hpp"

static_assert(sizeof(Effect) == sizeof(uint8_t), "Effect must be one byte on AVR");

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
    test.expectEqIdx(actual.attackMask, expected.attackMask, F("creature.attackMask"), index);
}

// First-stage evolution edges (jp8.11.11). The packed Evolution mapping must
// match the model fixture, name distinct endpoints, and carry the same
// threshold as the packed source creature's evoLevel.
void test_evolution_edges(FxTest &test) {
    constexpr uint8_t kEdgeCount = EvolutionFixture::evolutionEdgeCount;
    static_assert(kEdgeCount > 0, "first-beta evolution mapping must not be empty");

    uint8_t source[kEdgeCount];
    uint8_t target[kEdgeCount];
    uint8_t level[kEdgeCount];
    uint8_t expectedSource[kEdgeCount];
    uint8_t expectedTarget[kEdgeCount];
    uint8_t expectedLevel[kEdgeCount];
    FX::readDataBytes(Evolution::evolutionSource, source, sizeof(source));
    FX::readDataBytes(Evolution::evolutionTarget, target, sizeof(target));
    FX::readDataBytes(Evolution::evolutionLevel, level, sizeof(level));
    memcpy_P(expectedSource, EvolutionFixture::evolutionSource, sizeof(expectedSource));
    memcpy_P(expectedTarget, EvolutionFixture::evolutionTarget, sizeof(expectedTarget));
    memcpy_P(expectedLevel, EvolutionFixture::evolutionLevel, sizeof(expectedLevel));

    for (uint8_t index = 0; index < kEdgeCount; ++index) {
        test.expectEqIdx(source[index], expectedSource[index], F("evolution.source"), index);
        test.expectEqIdx(target[index], expectedTarget[index], F("evolution.target"), index);
        test.expectEqIdx(level[index], expectedLevel[index], F("evolution.level"), index);
        test.expectEqIdx(source[index] != target[index], true, F("evolution.distinct"), index);
        test.expectEqIdx(level[index] >= 1, true, F("evolution.threshold"), index);
        const CreatureData_t seed = getCreatureFromStore(source[index]);
        test.expectEqIdx(seed.evoLevel, level[index], F("evolution.evoLevel"), index);
    }
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
    // The shipped encounter table remains limited to the original roster;
    // exercise its upper edge while the loop above checks all packed records.
    const uint8_t encounterIds[] = {0, 31};
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
        test.expectEqIdx(active.attackMask, expected.attackMask,
                         F("encounter.attackMask"), id);

        // The requested level initializes both encounter metadata and creature stats.
        test.expectEqIdx(active.level, encounterLevel, F("encounter.level0"), id);
        test.expectEqIdx(encounter.bench[side][0].level, 0, F("encounter.level1"), id);
        test.expectEqIdx(encounter.bench[side][1].level, 0, F("encounter.level2"), id);
        test.expectEqIdx(encounter.partyCount[side], 1, F("encounter.partyCount"), id);
    }

    test_evolution_edges(test);
}
