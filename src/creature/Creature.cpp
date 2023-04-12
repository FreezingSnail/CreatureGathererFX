
#include "Creature.hpp"
#include "opponent/OpponentSeed.hpp"

uint8_t seedToStat(uint8_t seed) {
	// Need to do some math here to scale a 4 bit number to 8
	return seed;
}

//This will need to load the creature seed from the progmemstore
CreatureSeed_t* getCreatureFromStore(uint8_t id) {
	CreatureSeed_t* ptr;
	return ptr;
}


Creature::Creature() {
	this->statlist.attack = 0;
	this->statlist.defense = 0;
	this->statlist.speed = 0;
	this->statlist.health = 0;
}

void Creature::load(CreatureSeed_t* seed) {
	this->seed = seed;
	this->setStats();
	//Need some kind of default setting for moves ?

}

//00,id1,lvl1,move11,move12,move13,move14,
void Creature::loadFromOpponentSeed(uint32_t seed){
	CreatureSeed_t* cSeed = getCreatureFromStore(parseOpponentCreatureSeedID(seed));
	this->load(cSeed);
	this->level = parseOpponentCreatureSeedlvl(seed);
	this->setMove(parseOpponentCreatureSeedMove(seed, 0), 0);
	this->setMove(parseOpponentCreatureSeedMove(seed, 1), 1);
	this->setMove(parseOpponentCreatureSeedMove(seed, 2), 2);
	this->setMove(parseOpponentCreatureSeedMove(seed, 3), 3);
	this->setStats();
}

void Creature::changeMove(uint8_t slot, uint8_t newMove) {
	this->moves[slot] = newMove;
}

void Creature::setStats() {
	this->statlist.attack = seedToStat(this->getAtkStatSeed());
	this->statlist.defense = seedToStat(this->getDefStatSeed());
	this->statlist.speed = seedToStat(this->getSpdStatSeed());
	this->statlist.health = seedToStat(this->getHpStatSeed());
}

// should prob have error checking but w/e
void Creature::setMove(uint8_t move, uint8_t slot){
	this->moves[slot] = move;

}
// some ai to find best advantage should move this out of this class though
uint8_t Creature::getAdvantage(Type_t opponent) {
	return 0;
}

uint8_t Creature::getMove(uint8_t slot) {
	return this->moves[slot];
}

uint8_t Creature::getLevel() {
	return this->level;
}

uint8_t Creature::getAtkStatSeed() {
	return (this->seed->statSeed & 0b1111000000000000) >> 12;
}

uint8_t Creature::getDefStatSeed() {
	return (this->seed->statSeed & 0b0000111100000000) >> 8;
}

uint8_t Creature::getHpStatSeed() {
	return (this->seed->statSeed & 0b00000000111100000000) >> 4;
}

uint8_t Creature::getSpdStatSeed() {
	return (this->seed->statSeed & 0b0000000000001111);
}

uint8_t Creature::getAtkStat() {
	return this->statlist.attack;
}
uint8_t Creature::getDefStat() {
	return this->statlist.defense;
}
uint8_t Creature::getHpStat() {
	return this->statlist.health;
}
uint8_t Creature::getSpdStat() {
	return this->statlist.speed;
}


#ifdef DEBUG
#include <iostream>
void Creature::printStats() {
	std::cout << "atk: " << this->getAtkStat() << "def: " << this->getDefStat() << "spd: " << this->getSpdStat() << "hp: " << this->getHpStat() <<std::endl;
}
#endif