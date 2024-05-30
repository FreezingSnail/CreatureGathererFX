#pragma once
#include "test.hpp"

#include "../src/lib/ReadData.hpp"
#include "../src/player/Player.hpp"

void PlayerTest(TestSuite &t) {
    Test test = Test(__func__);
    Player player = Player();
    player.basic();
    test.assert(static_cast<int>(player.party[0].id), 0, "Player Creature 1 ID");
    test.assert(static_cast<int>(player.party[1].id), 1, "Player Creature 2 ID");
    test.assert(static_cast<int>(player.party[2].id), 2, "Player Creature 3 ID");
    test.assert(static_cast<int>(player.party[0].level), 31, "Player Creature 1 Level");

    t.addTest(test);
}

void PlayerSuite(TestRunner &r) {
    TestSuite t = TestSuite("Player Suite");
    PlayerTest(t);
    r.addTestSuite(t);
}