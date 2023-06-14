#pragma once
#include "../../creature/Creature.hpp"
#include "../../item/Item.hpp"

typedef enum State { WORLD, BATTLE, ARENA } GameState_t;

// class GameState {
// private:
//   Creature OwnedMons[32];
//   Creature *currentParty[3];
//   uint8_t trophies; // bitarray for competeion tracking
//   Item items[8];
// };