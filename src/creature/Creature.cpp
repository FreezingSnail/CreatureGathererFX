#include "Creature.hpp"
#include "../opponent/OpponentSeed.hpp"
#include "../data/Creatures.hpp"
#include "../lib/Move.hpp"
#include "../sprites/sprites.hpp"

uint8_t seedToStat(uint8_t seed) {
	// Need to do some math here to scale a 4 bit number to 8
	return seed;
}

//This will need to load the creature seed from the progmemstore
CreatureSeed_t getCreatureFromStore(uint8_t id) {
	CreatureSeed_t seed;
	memcpy_P(&seed, &CreatureData[0], sizeof(CreatureSeed_t));
	return seed;
	//return nullptr;
}


Creature::Creature() {
	this->statlist.attack = 0;
	this->statlist.defense = 0;
	this->statlist.speed = 0;
	this->statlist.health = 0;
}

void Creature::load(CreatureSeed_t seed) {
	this->seed = seed;
	this->type = (Type_t)(seed.creatureID >> 3);
	this->setStats();
	this->loadMoves();
	//Need some kind of default setting for moves ?

	this->loadSprite();

}

//00,id1,lvl1,move11,move12,move13,move14,
void Creature::loadFromOpponentSeed(uint32_t seed){
	CreatureSeed_t cSeed = getCreatureFromStore(parseOpponentCreatureSeedID(seed));
	this->seed = cSeed;
	this->type = (Type_t)(cSeed.creatureID >> 3);
	this->setStats();
	this->level = parseOpponentCreatureSeedlvl(seed);
	this->setMove(parseOpponentCreatureSeedMove(seed, 0), 0);
	this->setMove(parseOpponentCreatureSeedMove(seed, 1), 1);
	this->setMove(parseOpponentCreatureSeedMove(seed, 2), 2);
	this->setMove(parseOpponentCreatureSeedMove(seed, 3), 3);

	this->loadSprite();
}

void Creature::loadMoves() {
	this->setMove(this->seed.startingMoves & 0b11111, 0);
	this->setMove((this->seed.startingMoves >> 5) & 0b11111, 1);
	this->setMove((this->seed.startingMoves >> 10) & 0b11111, 2);
	this->setMove((this->seed.startingMoves >> 15) & 0b11111, 3);
}

void Creature::loadSprite() {
	//this will need to take the creatureID to pull its sprite from the sheet
	uint8_t id = this->seed.creatureID & 0b11111;

	//force this for now since no sheet
	this->sprite = snailSprite;
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
}

// some ai to find best advantage should move this out of this class though
// going to need this for the opponent ai
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
	return (this->seed.statSeed & 0b1111000000000000) >> 12;
}

uint8_t Creature::getDefStatSeed() {
	return (this->seed.statSeed & 0b0000111100000000) >> 8;
}

uint8_t Creature::getHpStatSeed() {
	return (this->seed.statSeed & 0b0000000011110000) >> 4;
}

uint8_t Creature::getSpdStatSeed() {
	return (this->seed.statSeed & 0b0000000000001111);
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

uint8_t Creature::getID() {
	return this->seed.creatureID & 0b00011111;
}

bool Creature::moveTypeBonus(uint8_t move) {
	return this->type == (Type_t)getMoveType(move);
}