#include "ReadData.hpp"
#include <ArduboyFX.h>
#include "../fxdata.h"

MoveBitSet getMovePack(uint8_t index) {
    MoveBitSet move;
    uint24_t rowAddress = MoveData::movePack + sizeof(MoveBitSet) * index;
    FX::readDataObject(rowAddress, move);
    return move;
}

// TODO: The rates dont exist yet in flash data
uint8_t getEffectRate(Effect effect) {
    uint8_t rate;
    // uint24_t rowAddress = MoveData::effectRates + sizeof(uint8_t) * static_cast<uint8_t>(effect);
    // FX::readDataObject(rowAddress, rate);
    // return rate;
    return 100;
}

// TODO: add a bitarray for the effect targets
bool selfEffect(Effect effect) {
    uint32_t effectTargets;
    // FX::readDataObject(MoveData::selfEffect, effectTargets);
    return effectTargets >> uint8_t(effect) & 1 == 1;
}

OpponentSeed readOpponentSeed(uint8_t index) {
    OpponentSeed seed = OpponentSeed{0, 0, 1};
    uint24_t rowAddress = FX::readIndexedUInt24(opts, index);
    FX::readDataObject(rowAddress, seed);
    return seed;
}

CreatureData_t getCreatureFromStore(uint8_t id) {
    CreatureData_t cseed;
    uint24_t rowAddress = CreatureData::creatureData + (sizeof(CreatureData_t) * id);
    FX::readDataObject(rowAddress, cseed);
    return cseed;
}

// todo(snail)
//  maybe I should move creature out into an abstraction so its easier to change
void load(Creature *creature, CreatureData_t seed) {
    creature->id = static_cast<uint8_t>((seed.id));
    // TODO: for testing
    creature->level = 31;
    creature->loadTypes(seed);
    creature->setStats(seed);
    // Need some kind of default setting for moves ?
    creature->loadMoves(seed);
    // creature->loadSprite(seed);
}

// 00,id1,lvl1,move11,move12,move13,move14,

void arenaLoad(Creature *creature, uint24_t addr, uint8_t lvl) {
    uint8_t data[4];
    data[0] = FX::readIndexedUInt8(addr, 1);
    data[1] = FX::readIndexedUInt8(addr, 2);
    data[2] = FX::readIndexedUInt8(addr, 3);
    data[3] = FX::readIndexedUInt8(addr, 4);

    creature->id = FX::readIndexedUInt8(addr, 0);
    CreatureData_t cSeed = getCreatureFromStore(creature->id);

    creature->loadTypes(cSeed);
    creature->level = lvl;
    creature->setStats(cSeed);
    creature->setMove(FX::readIndexedUInt8(addr, 1), 0);
    creature->setMove(FX::readIndexedUInt8(addr, 2), 1);
    creature->setMove(FX::readIndexedUInt8(addr, 3), 2);
    creature->setMove(FX::readIndexedUInt8(addr, 4), 3);
}

void ReadOpt(Opponent *opt, uint8_t index) {
    uint24_t addr = opponent_seeds + sizeof(OpponentSeed) * index;
    OpponentSeed seed;
    FX::readDataObject(addr, seed);
    opt->loadOpt(&seed);
}

void loadEncounterOpt(Opponent *opt, uint8_t id, uint8_t level) {
    CreatureData_t cseed;
    uint24_t rowAddress = CreatureData::creatureData + (sizeof(CreatureData_t) * id);
    FX::readDataObject(rowAddress, cseed);
    opt->levels[0] = 2;
    opt->levels[1] = 0;
    opt->levels[2] = 0;
    load(&opt->party[0], cseed);
    //  this->party[1].load(eseed);
    //  this->party[2].load(eseed);
}
