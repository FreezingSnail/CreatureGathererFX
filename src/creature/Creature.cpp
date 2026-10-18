#include "Creature.hpp"

#include "../lib/DataTypes.hpp"
#include "../lib/ReadData.hpp"
#include "LevelCurve.hpp"

// This will need to load the creature seed from the progmemstore
Creature::Creature() {
    // Deterministic empty state: a default Creature is an empty party slot
    // (BattleSetup derives slot occupancy from level), so reset identity too.
    this->id = 0;
    this->level = 0;
    this->hp = 0;
    this->attackMask = 0;
}

void Creature::load(CreatureData_t seed) {
    id = static_cast<uint8_t>((seed.id));
    // Legacy species-only callers have no experience. Keep their current
    // maximum-level setup until they load a StoreRecord.
    level = levelFromExp(levelExpThreshold(LEVEL_COUNT - 1));
    loadTypes(seed);
    setStats(seed);
    attackMask = seed.attackMask;
    // loadSprite(seed);
}

void Creature::load(const StoreRecord &record) {
    id = record.id;
    level = levelFromExp(record.exp);
    CreatureData_t seed = getCreatureFromStore(id);
    loadTypes(seed);
    setStats(seed);
    attackMask = record.attackMask;
}

void Creature::loadFromOpponentSeed(CreatureSeed seed) {
    id = seed.id;
    CreatureData_t cSeed = getCreatureFromStore(id);
    loadTypes(cSeed);
    level = seed.lvl;
    setStats(cSeed);
    attackMask = seed.attackMask;
    // loadSprite(cSeed);
}

// void Creature::loadSprite(CreatureData_t seed) {
//   this->sprite = creatureSprites[static_cast<uint8_t>((seed.id))];
// }

void Creature::loadTypes(CreatureData_t seed) {
    this->types = DualType((Type) static_cast<uint8_t>((seed.type1)), (Type) static_cast<uint8_t>((seed.type2)));
}

void Creature::setStats(CreatureData_t seed) {
    // need to bulk up lineraly
    this->hp = seedToStat((seed.hpSeed)) + 30;
}

// some ai to find best advantage should move this out of this class though
// going to need this for the opponent ai
uint8_t Creature::getAdvantage(DualType opponent) {
    return 0;
}

uint8_t Creature::seedToStat(uint8_t seed) {
    return 2 * level + (seed * (level / 3));
    // return (2*this->level)*(seed/3);
}
