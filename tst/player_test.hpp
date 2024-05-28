#pragma once
#include "test.hpp"

#include "../src/lib/ReadData.hpp"
#include "../src/player/Player.hpp"

void PlayerTest() {
    Test test;
    Player player = Player();
    player.basic();
    test.assert(static_cast<int>(player.party[0].id), 0, "Player Creature 1 ID");
    test.assert(static_cast<int>(player.party[1].id), 1, "Player Creature 2 ID");
    test.assert(static_cast<int>(player.party[2].id), 2, "Player Creature 3 ID");
    test.assert(static_cast<int>(player.party[0].level), 31, "Player Creature 1 Level");

    test.printSummary("Player Load Test");
}

void PlayerSuite() {
    printHeader("Player Suite");
    PlayerTest();
}