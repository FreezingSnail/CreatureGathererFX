#pragma once
#include "test.hpp"
#include "../src/creature/Creature.hpp"
#include "../src/creature/LevelCurve.hpp"
#include "../src/creature/Evolution.hpp"
#include "../src/lib/ReadData.hpp"
#include "src/FXDataFake.hpp"
#include <string.h>
static_assert(sizeof(Effect) == sizeof(uint8_t), "Effect must use one byte");

void CreatureLoadTest(TestSuite &t) {
    Test test = Test(__func__);
    Creature creature = Creature();
    creature.load(getCreatureFromStore(0));
    test.assert(creature.id, 0, "Creature ID");
    test.assert(creature.level, 31, "Creature Level");
    test.assert(creature.types.getType1(), static_cast<int>(Type::WIND), "Creature Type 1");
    test.assert(creature.types.getType2(), static_cast<int>(Type::NONE), "Creature Type 2");
    test.assert(creature.attackMask, 2, "Creature attack mask (water)");
    test.assert(creature.hp, creature.seedToStat(3) + 30, "Creature HP");

    t.addTest(test);
}

void CreatureLoadFromOpponnetSeed(TestSuite &t) {
    Test test = Test(__func__);
    Creature creature = Creature();
    creature.loadFromOpponentSeed({0, 31, 2});
    test.assert(creature.id, 0, "Creature ID");
    test.assert(creature.level, 31, "Creature Level");
    test.assert(creature.types.getType1(), static_cast<int>(Type::WIND), "Creature Type 1");
    test.assert(creature.types.getType2(), static_cast<int>(Type::NONE), "Creature Type 2");
    test.assert(creature.attackMask, 2, "Opponent seed attack mask");

    creature.loadFromOpponentSeed({0, 17, 4});
    test.assert(creature.level, 17, "Creature uses opponent seed level");
    test.assert(creature.attackMask, 4, "Opponent seed attack mask follows the seed");
    test.assert(creature.hp,
                static_cast<uint8_t>(2 * 17 + 3 * (17 / 3) + 30),
                "Opponent HP uses the seed level");

    t.addTest(test);
}

void CreatureStoredRecordTest(TestSuite &t) {
    Test test = Test(__func__);
    StoreRecord record = {0, 27, 2, {0, 0, 0}, 0};
    Creature creature;
    creature.load(record);
    test.assert(creature.id, record.id, "Stored creature ID");
    test.assert(creature.level, 3, "Stored experience derives level");
    test.assert(creature.attackMask, record.attackMask, "Stored mask overrides species default");
    test.assert(creature.hp, creature.seedToStat(3) + 30,
                "Stored creature HP uses derived level");
    t.addTest(test);
}

void CreatureLevelCurveTest(TestSuite &t) {
    Test test = Test(__func__);
    test.assert(levelFromExp(0), 1, "Zero experience has minimum level");
    for (uint8_t level = 0; level < LEVEL_COUNT; ++level) {
        const uint16_t threshold = levelExpThreshold(level);
        const uint16_t expectedThreshold = static_cast<uint16_t>(level) * level * level;
        const uint8_t expectedLevel = level == 0 ? 1 : level;
        test.assert(threshold, expectedThreshold,
                    "Cubic threshold " + std::to_string(level));
        test.assert(levelFromExp(threshold), expectedLevel,
                    "At threshold " + std::to_string(level));
        if (threshold > 0) {
            const uint8_t previousLevel = level <= 2 ? 1 : level - 1;
            test.assert(levelFromExp(threshold - 1), previousLevel,
                        "Below threshold " + std::to_string(level));
        }
    }
    test.assert(levelFromExp(65535), LEVEL_COUNT - 1,
                "Experience above the final threshold caps at maximum level");
    t.addTest(test);
}

// jp8.11.12: the packed first-beta evolution mapping resolves only the three
// authored edges at their authored threshold; species id adjacency is never
// inferred and no threshold-less or unrelated species ever matches. The packed
// arrays are declared contiguously (source, target, level), so the host fixture
// derives each column's offset from the published addresses rather than a
// literal.
void installEvolutionEdges() {
    const uint32_t targetOffset = static_cast<uint32_t>(
        Evolution::evolutionTarget - Evolution::evolutionSource);
    const uint32_t levelOffset = static_cast<uint32_t>(
        Evolution::evolutionLevel - Evolution::evolutionSource);
    fxDataFake::dataBase = Evolution::evolutionSource;
    memset(fxDataFake::dataBytes, 0, sizeof(fxDataFake::dataBytes));
    // Authored edges 0->1, 3->4, 6->7 at level 8.
    fxDataFake::dataBytes[0] = 0;
    fxDataFake::dataBytes[1] = 3;
    fxDataFake::dataBytes[2] = 6;
    fxDataFake::dataBytes[targetOffset + 0] = 1;
    fxDataFake::dataBytes[targetOffset + 1] = 4;
    fxDataFake::dataBytes[targetOffset + 2] = 7;
    fxDataFake::dataBytes[levelOffset + 0] = 8;
    fxDataFake::dataBytes[levelOffset + 1] = 8;
    fxDataFake::dataBytes[levelOffset + 2] = 8;
}

void CreatureEvolutionEdgeTest(TestSuite &t) {
    Test test = Test(__func__);
    installEvolutionEdges();
    test.assert(CreatureEvolution::kEdgeCount, static_cast<uint8_t>(3),
                "three authored first-beta edges");
    test.assert(CreatureEvolution::targetFor(0, 8), static_cast<uint8_t>(1),
                "species 0 -> 1 at its threshold");
    test.assert(CreatureEvolution::targetFor(3, 8), static_cast<uint8_t>(4),
                "species 3 -> 4 at its threshold");
    test.assert(CreatureEvolution::targetFor(6, 31), static_cast<uint8_t>(7),
                "species 6 -> 7 above its threshold");
    test.assert(CreatureEvolution::targetFor(0, 7), static_cast<uint8_t>(0xFF),
                "below the threshold no edge fires");
    // No inferred id adjacency: neighbours of the sources never evolve.
    test.assert(CreatureEvolution::targetFor(1, 31), static_cast<uint8_t>(0xFF),
                "species 1 has no authored edge");
    test.assert(CreatureEvolution::targetFor(2, 31), static_cast<uint8_t>(0xFF),
                "species 2 has no authored edge");
    test.assert(CreatureEvolution::targetFor(4, 31), static_cast<uint8_t>(0xFF),
                "species 4 has no authored edge");
    test.assert(CreatureEvolution::targetFor(63, 31), static_cast<uint8_t>(0xFF),
                "an unrelated species has no authored edge");
    t.addTest(test);
}

void CreatureSuite(TestRunner &r) {
    TestSuite t = TestSuite("Creature Suite");
    CreatureLoadTest(t);
    CreatureLoadFromOpponnetSeed(t);
    CreatureStoredRecordTest(t);
    CreatureLevelCurveTest(t);
    CreatureEvolutionEdgeTest(t);
    r.addTestSuite(t);
}
