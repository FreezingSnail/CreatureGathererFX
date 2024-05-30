#pragma once
#include "test.hpp"

#include "../src/lib/ReadData.hpp"
#include "../src/opponent/Opponent.hpp"

void OpponentTest(TestSuite &t) {
    Test test = Test(__func__);
    Opponent opponent = Opponent();
    OpponentSeed_t seed = readOpponentSeed(0);
    opponent.loadOpt(&seed);
    test.assert(static_cast<int>(opponent.party[0].id), 1, "Opponent Creature 1 ID");
    test.assert(static_cast<int>(opponent.party[1].id), 1, "Opponent Creature 2 ID");
    test.assert(static_cast<int>(opponent.party[2].id), 1, "Opponent Creature 3 ID");
    test.assert(static_cast<int>(opponent.levels[0]), 2, "Opponent Creature 1 Level");
    test.assert(static_cast<int>(opponent.levels[1]), 1, "Opponent Creature 2 Level");
    test.assert(static_cast<int>(opponent.levels[2]), 1, "Opponent Creature 3 Level");
    test.assert(static_cast<int>(opponent.party[0].level), 2, "Opponent Creature 1  Creature Level");
    test.assert(static_cast<int>(opponent.party[1].level), 1, "Opponent Creature 2 Creature Level");
    test.assert(static_cast<int>(opponent.party[2].level), 1, "Opponent Creature 3 Creature Level");

    t.addTest(test);
}

void OpponentSuite(TestRunner &r) {
    TestSuite t = TestSuite("Opponent Suite");
    OpponentTest(t);
    r.addTestSuite(t);
}