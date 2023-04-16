
#include "Player.hpp"

Player::Player() {
}

void Player::setMonster(uint8_t index, CreatureSeed_t* seed) {
    this->party[index].load(seed);
}