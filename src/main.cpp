#include "lib/TypeTable.hpp"
#include <iostream>
#include <stdint.h>
#include "engine/battle/Battle.hpp" 
#include "player/Player.hpp"
#include "data/Creatures.hpp"
#include "data/opponentsData.h"

int main() {
    Player player = Player();
    CreatureSeed_t cseed = CreatureData[0];
    player.setMonster(1, &cseed);
    BattleEngine engine = BattleEngine();
    engine.encounter(&player, &op1);
    return 0;
}