#include "Battle.hpp"
#include <iostream>
#include <map>
#include <stdint.h>

//-exec type format add --format u 'unsigned char'
static void print_map(const std::map<int, int> &m) {
    // Iterate using C++17 facilities
    for (const auto &[key, value] : m)
        std::cout << '[' << key << "] = " << value << "; " << std::endl;
}

int main() {
    srand(time(NULL));
    BattleEngine engine;
    Player p;
    std::map<int, int> winRates;
    for (int i = 0; i < 19; i++) {
        winRates[i] = 0;
    }
    for (long i = 0; i < 1000000; i++) {
        int pindex = rand() % 19;
        int oindex = rand() % 19;
        p.Read(pindex);
        engine.startArena(p, oindex);
        while (engine.activeBattle) {
            engine.encounter(p);
            if (engine.checkWin()) {
                winRates[pindex]++;
            }
            if (engine.checkLoss()) {
                winRates[oindex]++;
            }
        }
    }
    print_map(winRates);
}