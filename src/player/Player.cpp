
#include "Player.hpp"
#include "../data/Creatures.hpp"

Player::Player() {
    CreatureData_t cseed;
    memcpy_P(&cseed, &CreatureData[0], sizeof(CreatureData_t));
    this->party[0].level = 1;
    this->party[1].level = 1;
    this->party[2].level = 1;
    this->setMonster(0, cseed);
    this->setMonster(1, cseed);
    this->setMonster(2, cseed);
}

void Player::setMonster(uint8_t index, CreatureData_t seed) {
    this->party[index].load(seed);
}