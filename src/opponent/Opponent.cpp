
#include "Opponent.hpp"
#include "../data/Creatures.hpp"
#include "../fxdata.h"
#include "OpponentSeed.hpp"
#include <ArduboyFX.h>

Opponent::Opponent() {
}
#define dbf __attribute__((optimize("-O0")))

void Opponent::load(OpponentSeed_t *seed) {
    this->levels[0] = 5;   // parseOpponentCreatureSeedlvl(seed->firstCreature);
    this->levels[1] = parseOpponentCreatureSeedlvl(seed->secondCreature);
    this->levels[2] = parseOpponentCreatureSeedlvl(seed->thirdCreature);
    this->party[0].loadFromOpponentSeed(seed->firstCreature);
    this->party[1].loadFromOpponentSeed(seed->secondCreature);
    this->party[2].loadFromOpponentSeed(seed->thirdCreature);
}

void Opponent::Read(uint8_t index) {
    uint24_t creatures[3];
    uint24_t addr = Teams::teamList + sizeof(uint24_t) * 7 * index;
    creatures[0] = FX::readIndexedUInt24(addr, 0);
    creatures[1] = FX::readIndexedUInt24(addr, 1);
    creatures[2] = FX::readIndexedUInt24(addr, 2);
    levels[0] = uint8_t(FX::readIndexedUInt24(addr, 3));
    levels[1] = uint8_t(FX::readIndexedUInt24(addr, 4));
    levels[2] = uint8_t(FX::readIndexedUInt24(addr, 5));
    party[0].arenaLoad(creatures[0], levels[0]);
    party[1].arenaLoad(creatures[1], levels[1]);
    party[2].arenaLoad(creatures[2], levels[2]);

    nameptr = FX::readIndexedUInt24(addr, 6);
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
