#pragma once
#include "test.hpp"
#include "../src/creature/Creature.hpp"
#include "../src/creature/LevelCurve.hpp"
#include "../src/lib/ReadData.hpp"
#include "src/FXDataFake.hpp"
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
    test.assert(creature.statlist.attack, creature.seedToStat(3), "Creature Attack");
    test.assert(creature.statlist.defense, creature.seedToStat(3), "Creature Defense");
    test.assert(creature.statlist.speed, creature.seedToStat(3), "Creature Speed");
    test.assert(creature.statlist.hp, creature.seedToStat(3) + 30, "Creature HP");
    test.assert(creature.statlist.spcAtk, creature.seedToStat(2), "Creature Special Attack");
    test.assert(creature.statlist.spcDef, creature.seedToStat(2), "Creature Special Defense");

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
    test.assert(creature.statlist.attack,
                static_cast<uint8_t>(2 * 17 + 3 * (17 / 3)),
                "Opponent stats use the seed level");

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
    test.assert(creature.statlist.attack, creature.seedToStat(3),
                "Stored creature stats use derived level");
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

void CreatureSuite(TestRunner &r) {
    TestSuite t = TestSuite("Creature Suite");
    CreatureLoadTest(t);
    CreatureLoadFromOpponnetSeed(t);
    CreatureStoredRecordTest(t);
    CreatureLevelCurveTest(t);
    r.addTestSuite(t);
}
