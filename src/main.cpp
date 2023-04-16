#include "lib/TypeTable.hpp"
#include <iostream>
#include <stdint.h>
#include "engine/battle/Battle.hpp" 
#include "player/Player.hpp"
#include "data/opponentsData.h"


int main() {
    Player player = Player();
    BattleEngine engine = BattleEngine();
    engine.encounter(&player, &op1);
    return 0;
}