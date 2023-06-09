#pragma once
#include <stdint.h>

#include "../creature/Creature.hpp"
#include "../data/Creatures.hpp"
#include "../item/Item.hpp"

class Player {
  struct caughtCreature {
    uint8_t id;
    uint8_t level;
  };

 public:
  Creature party[3];
  uint8_t creatureHPs[3];
  Item items[10];

  caughtCreature storedCreatures[6];

  Player();
  void setMonster(uint8_t index, CreatureData_t seed);
  void storeCreature(uint8_t slot, uint8_t id, uint8_t level);
};