
#include "Opponent.hpp"
#include "../data/Creatures.hpp"
#include "../fxdata/fxdata.h"
#include "OpponentSeed.hpp"
#include <ArduboyFX.h>

Opponent::Opponent() {}
#define dbf __attribute__((optimize("-O0")))

void Opponent::load(OpponentSeed_t *seed) {
    this->levels[0] = 5;   // parseOpponentCreatureSeedlvl(seed->firstCreature);
    this->levels[1] = parseOpponentCreatureSeedlvl(seed->secondCreature);
    this->levels[2] = parseOpponentCreatureSeedlvl(seed->thirdCreature);
    this->party[0].loadFromOpponentSeed(seed->firstCreature);
    this->party[1].loadFromOpponentSeed(seed->secondCreature);
    this->party[2].loadFromOpponentSeed(seed->thirdCreature);
}

struct teamAddrs {
    uint24_t a;
    uint24_t b;
    uint24_t c;
};

void dbf Opponent::Read(uint8_t index) {
    teamAddrs creatures;
    uint24_t addr = Teams::teamList + sizeof(uint24_t) * 3 * index;
    // FX::readDataObject(addr, creatures);
    creatures.a = FX::readIndexedUInt24(addr, 0);
    creatures.b = FX::readIndexedUInt24(addr, 1);
    creatures.c = FX::readIndexedUInt24(addr, 2);
    party[0].arenaLoad(creatures.a);
    party[1].arenaLoad(creatures.b);
    party[2].arenaLoad(creatures.c);
}

void __attribute__((optimize("-O0"))) Opponent::loadEncounterOpt(uint8_t id, uint8_t level) {
    CreatureData_t cseed;
    uint24_t rowAddress = CreatureData::creatureData + (sizeof(CreatureData_t) * id);
    FX::readDataObject(rowAddress, cseed);
    this->levels[0] = 2;
    this->levels[1] = 0;
    this->levels[2] = 0;
    this->party[0].load(cseed);
    //  this->party[1].load(eseed);
    //  this->party[2].load(eseed);
}
