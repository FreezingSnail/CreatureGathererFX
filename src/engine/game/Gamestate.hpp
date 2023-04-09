#pragma once
#include "monster/Monster.hpp"
#include "item/Item.hpp"

class GameState {
    private:
    Monster OwnedMons[32];
    Monster* currentParty[3];
    uint8_t trophies; //bitarray for competeion tracking
    Item items[8];
};