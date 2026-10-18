
#include "Player.hpp"

#include "../lib/ReadData.hpp"

Player::Player() {
    // A fresh Player owns an empty bag. Static-storage globals are already
    // zero, but an explicit clear keeps Player() value-initialization
    // deterministic for a save taken before any load or fresh-game init.
    item::inventoryClear(this->inventory);
    item::keyItemsClear(this->keyItems);
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

void Player::basic() {
    // One persistent bag serves the world and any future battle item use.
    item::setBattleInventory(&this->inventory);
    loadCreature(0, 0);
    loadCreature(1, 1);
    loadCreature(2, 2);
}

void Player::loadCreature(uint8_t index, uint8_t creatureIndex) {
    this->setCreature(index, getCreatureFromStore(creatureIndex));
}

void Player::setCreature(uint8_t index, CreatureData_t seed) {
    this->party[index].load(seed);
    // Player owns current HP between battle transitions. Installing a new
    // creature is the only path that initializes its full persistent HP.
    this->creatureHPs[index] = this->party[index].hp;
}

bool Player::healParty() {
    bool changed = false;
    for (uint8_t index = 0; index < 3; ++index) {
        // 255 is the empty/tombstone slot id; every authored species is 0..63.
        if (this->party[index].id == 255) continue;
        const uint8_t maxHP = this->party[index].hp;
        if (this->creatureHPs[index] != maxHP) changed = true;
        this->creatureHPs[index] = maxHP;
        this->party[index].status.clearEffects();
    }
    return changed;
}

void Player::restore(const Creature *savedParty, const uint8_t *savedHP) {
    for (uint8_t index = 0; index < 3; ++index) {
        this->party[index] = savedParty[index];
        const uint8_t maxHP = this->party[index].hp;
        this->creatureHPs[index] = savedHP[index] > maxHP ? maxHP : savedHP[index];
    }
}

void Player::storeCreature(uint8_t slot, uint8_t id, uint8_t level) {   // this->storedCreatures[slot] = caughtCreature{id, level};
}