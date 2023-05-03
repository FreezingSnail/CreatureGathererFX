
#include "Player.hpp"
#include "../data/Creatures.hpp"

Player::Player() {
    uint64_t cseed;
    memcpy_P(&cseed, &CreatureData[0], sizeof(uint64_t));
    this->setMonster(0, cseed);
    this->setMonster(1, cseed);
    this->setMonster(2, cseed);
    this->party[0].level = 1;
    this->party[1].level = 1;
    this->party[2].level = 1;
}

void Player::setMonster(uint8_t index, uint64_t seed) {
    this->party[index].load(seed);
}