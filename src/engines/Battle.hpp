#pragma once
#include "monster/Monster.hpp"

struct GameState {
    int playersMons[3];
};

class BattleEngine {
    private:
    GameState gameState;
    public: 
        BattleEngine();


};