#pragma once
#include "../../creature/Creature.hpp"
#include "../../item/Item.hpp"

enum GameState { OVERWORLD, FIGHT };

// class GameState {
// private:
//   Creature OwnedMons[32];
//   Creature *currentParty[3];
//   uint8_t trophies; // bitarray for competeion tracking
//   Item items[8];
// };