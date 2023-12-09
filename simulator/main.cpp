#include "Battle.hpp"
#include <stdint.h>

int main() {
    BattleEngine engine;
    Player p;
    uint8_t id = 1;
    p.Read(id);
    engine.startArena(p, id);
    while (engine.activeBattle) {
        engine.encounter(p);
    }
}