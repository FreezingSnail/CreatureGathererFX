#include "Creature.hpp"
#include "../opponent/OpponentSeed.hpp"
#include "../data/Creatures.hpp"
#include "../lib/Move.hpp"
#include "../sprites/sprites.hpp"
#include "../sprites/creatureSprites.h"

//This will need to load the creature seed from the progmemstore
uint64_t getCreatureFromStore(uint8_t id) {
	uint64_t seed;
	memcpy_P(&seed, &CreatureData[0], sizeof(uint64_t));
	return seed;
}


Creature::Creature() {
	this->statlist.attack = 0;
	this->statlist.defense = 0;
	this->statlist.speed = 0;
	this->statlist.health = 0;
	this->statlist.spcAtk = 0;
	this->statlist.spcDef = 0;
}

//todo(snail)
// maybe I should move this out into an abstraction so its easier to change
void Creature::load(uint64_t seed) {
	this->seed = seed;
	this->setStats();
	//Need some kind of default setting for moves ?
	this->loadMoves();
	this->loadSprite();
}

//00,id1,lvl1,move11,move12,move13,move14,
void Creature::loadFromOpponentSeed(uint32_t seed){
	uint64_t cSeed = getCreatureFromStore(parseOpponentCreatureSeedID(seed));
	this->seed = cSeed;
	this->loadTypes();
	this->setStats();
	this->level = parseOpponentCreatureSeedlvl(seed);
	this->setMove(parseOpponentCreatureSeedMove(seed, 0), 0);
	this->setMove(parseOpponentCreatureSeedMove(seed, 1), 1);
	this->setMove(parseOpponentCreatureSeedMove(seed, 2), 2);
	this->setMove(parseOpponentCreatureSeedMove(seed, 3), 3);
	this->loadSprite();
}

void Creature::loadMoves() {
	this->setMove((this->seed >> 19 & 0b11111111), 0);
	this->setMove((this->seed >> 14 & 0b11111111), 1);
	this->setMove((this->seed >> 9 & 0b11111111), 2);
	this->setMove((this->seed >> 4 & 0b11111111), 3);
}

void Creature::loadSprite() {
	this->sprite = creatureSprites[this->getID()];
}

void Creature::loadTypes() {
	uint8_t typeSeed = this->seed >> 56 & 0b111;
	uint8_t type2Seed = this->seed >> 53 & 0b111;
	if ( typeSeed == type2Seed) {
		this->types = DualType((Type)typeSeed);
	} else {
		this->types = DualType((Type)typeSeed, (Type)type2Seed);
	}
}

// should prob have error checking but w/e
void Creature::setMove(uint8_t move, uint8_t slot){
	this->moves[slot] = move;
}

void Creature::setStats() {
	this->statlist.attack = seedToStat(this->getAtkStatSeed());
	this->statlist.defense = seedToStat(this->getDefStatSeed());
	this->statlist.speed = seedToStat(this->getSpdStatSeed());
	this->statlist.health = seedToStat(this->getHpStatSeed());
	this->statlist.spcAtk = seedToStat(this->getSpcAtkStatSeed());
	this->statlist.spcDef = seedToStat(this->getSpcDefStatSeed());

}

// some ai to find best advantage should move this out of this class though
// going to need this for the opponent ai
uint8_t Creature::getAdvantage(DualType opponent) {
	return 0;
}

uint8_t Creature::getMove(uint8_t slot) {
	return this->moves[slot];
}

uint8_t Creature::getLevel() {
	return this->level;
}

uint8_t Creature::getAtkStatSeed() {
	return (this->seed >> 49 & 0b111);
}

uint8_t Creature::getDefStatSeed() {
	return (this->seed >> 45 & 0b1111);
}

uint8_t Creature::getHpStatSeed() {
	return (this->seed >> 33 & 0b1111);;
}

uint8_t Creature::getSpdStatSeed() {
	return (this->seed >> 29 & 0b1111);
}

uint8_t Creature::getSpcAtkStatSeed() {
	return (this->seed >> 41 & 0b1111);
}
uint8_t Creature::getSpcDefStatSeed() {
	return (this->seed >> 36 & 0b1111);
}

uint8_t Creature::getEvolutionLevel() {
	return (this->seed >> 24 & 0b11111111);
}

uint8_t Creature::getID() {
	return (this->seed >> 59 & 0b11111111);
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

uint8_t Creature::getSpcAtkStat() {
	return this->statlist.spcAtk;
}

uint8_t Creature::getSpcDefStat() {
	return this->statlist.spcDef;
}

bool Creature::moveTypeBonus(uint8_t move) {
	//return this->type1 == (Type_t)getMoveType(move) || this->type2 == (Type_t)getMoveType(move);
	return false;
}

uint8_t Creature::seedToStat(uint8_t seed) {
	return (2*this->level)*(seed/3);
}