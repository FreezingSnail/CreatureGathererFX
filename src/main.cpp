#include "lib/TypeTable.hpp"
#include <iostream>
#include <stdint.h>
#include "engine/battle/Battle.hpp" 

int main() {
    uint8_t result = getMatchupModifier(0, 1);
    std::cout << "matchup returned: " << unsigned(result) << "." << std::endl;
    CreatureSeed_t seed = CreatureSeed_t{0,0,0};
    BattleEngine engine = BattleEngine();
    return 0;
}