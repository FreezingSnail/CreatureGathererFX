
#include "Player.hpp"

#include "../data/Creatures.hpp"
#include "../fxdata/fxdata.h"

Player::Player() {
    // CreatureData_t cseed;
    // uint24_t rowAddress = FX::readIndexedUInt24(CreatureData::creatureData, 3);
    // FX::readDataObject(rowAddress, cseed);
    // this->party[0].level = 1;
    // this->setCreature(0, cseed);
    // rowAddress = FX::readIndexedUInt24(CreatureData::creatureData, 2);
    // FX::readDataObject(rowAddress, cseed);
    // this->party[1].level = 1;
    // this->setCreature(1, cseed);
    // rowAddress = FX::readIndexedUInt24(CreatureData::creatureData, 1);
    // FX::readDataObject(rowAddress, cseed);
    // this->party[2].level = 1;
    // this->setCreature(2, cseed);
}

void Player::loadreature(uint8_t index, uint8_t creatureIndex) {
    CreatureData_t cseed;
    uint24_t rowAddress = CreatureData::creatureData + (sizeof(CreatureData_t) * creatureIndex);
    FX::readDataObject(rowAddress, cseed);
    this->setCreature(index, cseed);
}

void Player::setCreature(uint8_t index, CreatureData_t seed) { this->party[index].load(seed); }

void Player::storeCreature(uint8_t slot, uint8_t id, uint8_t level) { this->storedCreatures[slot] = caughtCreature{id, level}; }