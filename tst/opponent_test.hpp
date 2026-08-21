#pragma once
#include <iostream>
#include "test.hpp"

#include "../src/lib/ReadData.hpp"
#include "../src/engine/battle/Battle.hpp"
#include "../src/opponent/Opponent.hpp"

void OpponentTest(TestSuite &t) {
    Test test = Test(__func__);
    Opponent opponent = Opponent();
    OpponentSeed seed = readOpponentSeed(0);
    test.addToLog("creature 1 id: " + std::to_string(seed.firstCreature.id) + " lvl " + std::to_string(seed.firstCreature.lvl) + " moves " +
                  std::to_string(seed.firstCreature.moves));
    test.addToLog("creature 2 id: " + std::to_string(seed.secondCreature.id) + " lvl " + std::to_string(seed.secondCreature.lvl) +
                  " moves " + std::to_string(seed.secondCreature.moves));
    opponent.loadOpt(&seed);
    test.assert(opponent.party[0].id, 0, "Opponent Creature 1 ID");
    test.assert(opponent.party[1].id, 3, "Opponent Creature 2 ID");
    test.assert(opponent.party[2].id, 6, "Opponent Creature 3 ID");
    test.assert(opponent.levels[0], 31, "Opponent Creature 1 Level");
    test.assert(opponent.levels[1], 31, "Opponent Creature 2 Level");
    test.assert(opponent.levels[2], 31, "Opponent Creature 3 Level");
    test.assert(opponent.party[0].level, 31, "Opponent Creature 1 Creature Level");
    test.assert(opponent.party[1].level, 31, "Opponent Creature 2 Creature Level");
    test.assert(opponent.party[2].level, 31, "Opponent Creature 3 Creature Level");

    BattleEngine encounter;
    constexpr uint8_t encounterCreature = 4;
    constexpr uint8_t encounterLevel = 17;
    encounter.startEncounter(encounterCreature, encounterLevel);
    CreatureData_t encounterSeed = getCreatureFromStore(encounterCreature);
    test.assert(encounter.opponent.party[0].id, encounterCreature, "Encounter Creature ID");
    test.assert(encounter.opponent.party[0].level, encounterLevel, "Encounter Creature Level");
    test.assert(encounter.opponent.levels[0], encounterLevel, "Encounter Level");
    test.assert(encounter.opponent.levels[1], 0, "Encounter Empty Party Level 1");
    test.assert(encounter.opponent.levels[2], 0, "Encounter Empty Party Level 2");
    test.assert(encounter.opponent.party[0].statlist.attack,
                2 * encounterLevel + encounterSeed.atkSeed * (encounterLevel / 3),
                "Encounter Creature Stats");

    t.addTest(test);
}

void OpponentSuite(TestRunner &r) {
    TestSuite t = TestSuite("Opponent Suite");
    OpponentTest(t);
    r.addTestSuite(t);
}