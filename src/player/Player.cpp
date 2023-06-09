
#include "Player.hpp"

#include "../data/Creatures.hpp"

Player::Player() {
  CreatureData_t cseed;
  memcpy_P(&cseed, &CreatureData[3], sizeof(CreatureData_t));
  this->party[0].level = 1;
  this->setMonster(0, cseed);
  memcpy_P(&cseed, &CreatureData[2], sizeof(CreatureData_t));
  this->party[1].level = 1;
  this->setMonster(1, cseed);
  memcpy_P(&cseed, &CreatureData[1], sizeof(CreatureData_t));
  this->party[2].level = 1;
  this->setMonster(2, cseed);
}

void Player::setMonster(uint8_t index, CreatureData_t seed) {
  this->party[index].load(seed);
}

void Player::storeCreature(uint8_t slot, uint8_t id, uint8_t level) {
  this->storedCreatures[slot] = caughtCreature{id, level};
}