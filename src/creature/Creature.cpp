#include "Creature.hpp"

#include "../lib/Move.hpp"
#include "../lib/DataTypes.hpp"
#include "../lib/ReadData.hpp"

// This will need to load the creature seed from the progmemstore
Creature::Creature() {
    this->statlist.attack = 0;
    this->statlist.defense = 0;
    this->statlist.speed = 0;
    this->statlist.hp = 0;
    this->statlist.spcAtk = 0;
    this->statlist.spcDef = 0;
}

void Creature::load(CreatureData_t seed) {
    id = static_cast<uint8_t>((seed.id));
    // TODO: for testing
    level = 31;
    loadTypes(seed);
    setStats(seed);
    // Need some kind of default setting for moves ?
    loadMoves(seed);
    // loadSprite(seed);
}

// 00,id1,lvl1,move11,move12,move13,move14,
void Creature::loadFromOpponentSeed(uint32_t seed) {
    id = parseOpponentCreatureSeedID(seed);
    CreatureData_t cSeed = getCreatureFromStore(id);
    loadTypes(cSeed);
    level = parseOpponentCreatureSeedlvl(seed);
    setStats(cSeed);
    setMove(parseOpponentCreatureSeedMove(seed, 0), 0);
    setMove(parseOpponentCreatureSeedMove(seed, 1), 1);
    setMove(parseOpponentCreatureSeedMove(seed, 2), 2);
    setMove(parseOpponentCreatureSeedMove(seed, 3), 3);
    // loadSprite(cSeed);
}

void Creature::loadMoves(CreatureData_t seed) {
    this->setMove(static_cast<uint8_t>((seed.move1)), 0);
    this->setMove(static_cast<uint8_t>((seed.move2)), 1);
    this->setMove(static_cast<uint8_t>((seed.move3)), 2);
    this->setMove(static_cast<uint8_t>((seed.move4)), 3);
}

// void Creature::loadSprite(CreatureData_t seed) {
//   this->sprite = creatureSprites[static_cast<uint8_t>((seed.id))];
// }

void dbf Creature::loadTypes(CreatureData_t seed) {
    this->types = DualType((Type) static_cast<uint8_t>((seed.type1)), (Type) static_cast<uint8_t>((seed.type2)));
}

// should prob have error checking but w/e
void Creature::setMove(uint8_t move, uint8_t slot) {
    this->moves[slot] = move;
    this->moveList[slot] = Move(getMovePack(move));
}

void Creature::setStats(CreatureData_t seed) {
    this->statlist.attack = seedToStat((seed.atkSeed));
    this->statlist.defense = seedToStat((seed.defSeed));
    this->statlist.speed = seedToStat((seed.spdSeed));
    // need to bulk up lineraly
    this->statlist.hp = seedToStat((seed.hpSeed)) + 30;
    this->statlist.spcAtk = seedToStat((seed.spcAtkSeed));
    this->statlist.spcDef = seedToStat((seed.spcDefSeed));
}

// some ai to find best advantage should move this out of this class though
// going to need this for the opponent ai
uint8_t Creature::getAdvantage(DualType opponent) {
    return 0;
}

uint8_t Creature::getMove(uint8_t slot) {
    return this->moves[slot];
}

bool Creature::moveTypeBonus(uint8_t index) {
    return this->types.getType1() == (Type)this->moveList[index].getMoveType() ||
           this->types.getType2() == (Type)this->moveList[index].getMoveType();
}

uint8_t Creature::seedToStat(uint8_t seed) {
    return 2 * level + (seed * (level / 3));
    // return (2*this->level)*(seed/3);
}
