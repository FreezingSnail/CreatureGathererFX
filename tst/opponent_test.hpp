#pragma once
#include <iostream>
#include "test.hpp"

#include "../src/lib/ReadData.hpp"
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

    t.addTest(test);
}

void OpponentSuite(TestRunner &r) {
    TestSuite t = TestSuite("Opponent Suite");
    OpponentTest(t);
    r.addTestSuite(t);
}