#pragma once
#include <iostream>
#include "test.hpp"

#include "../src/lib/ReadData.hpp"
#include "../src/creature/Creature.hpp"

void OpponentTest(TestSuite &t) {
    Test test = Test(__func__);
    OpponentSeed seed = readOpponentSeed(0);
    test.addToLog("creature 1 id: " + std::to_string(seed.firstCreature.id) + " lvl " + std::to_string(seed.firstCreature.lvl) + " moves " +
                  std::to_string(seed.firstCreature.moves));
    test.addToLog("creature 2 id: " + std::to_string(seed.secondCreature.id) + " lvl " + std::to_string(seed.secondCreature.lvl) +
                  " moves " + std::to_string(seed.secondCreature.moves));
    Creature first;
    Creature second;
    Creature third;
    first.loadFromOpponentSeed(seed.firstCreature);
    second.loadFromOpponentSeed(seed.secondCreature);
    third.loadFromOpponentSeed(seed.thirdCreature);
    test.assert(first.id, 0, "Opponent Creature 1 ID");
    test.assert(second.id, 3, "Opponent Creature 2 ID");
    test.assert(third.id, 6, "Opponent Creature 3 ID");
    test.assert(first.level, 31, "Opponent Creature 1 Level");
    test.assert(second.level, 31, "Opponent Creature 2 Level");
    test.assert(third.level, 31, "Opponent Creature 3 Level");

    constexpr uint8_t encounterCreature = 4;
    constexpr uint8_t encounterLevel = 17;
    Creature encounter;
    CreatureData_t encounterSeed = getCreatureFromStore(encounterCreature);
    encounter.id = encounterSeed.id;
    encounter.level = encounterLevel;
    encounter.loadTypes(encounterSeed);
    encounter.setStats(encounterSeed);
    encounter.loadMoves(encounterSeed);
    test.assert(encounter.id, encounterCreature, "Encounter Creature ID");
    test.assert(encounter.level, encounterLevel, "Encounter Creature Level");
    test.assert(encounter.statlist.attack,
                2 * encounterLevel + encounterSeed.atkSeed * (encounterLevel / 3),
                "Encounter Creature Stats");

    t.addTest(test);
}

void OpponentSuite(TestRunner &r) {
    TestSuite t = TestSuite("Opponent Suite");
    OpponentTest(t);
    r.addTestSuite(t);
}