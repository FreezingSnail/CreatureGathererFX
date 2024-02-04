#include "Creature.hpp"
#include <Arduboy2.h>
#include <avr/pgmspace.h>

#include "../data/Creatures.hpp"
#include "../fxdata.h"
#include "../lib/Move.hpp"
#include "../opponent/OpponentSeed.hpp"
#include "../common.hpp"
#include "../lib/Text.hpp"

#define dbf __attribute__((optimize("-O0")))

// This will need to load the creature seed from the progmemstore
const CreatureData_t getCreatureFromStore(uint8_t id) {
    CreatureData_t cseed;
    uint24_t rowAddress = CreatureData::creatureData + (sizeof(CreatureData_t) * id);
    FX::readDataObject(rowAddress, cseed);
    return cseed;
}

Creature::Creature() {
    this->statlist.attack = 0;
    this->statlist.defense = 0;
    this->statlist.speed = 0;
    this->statlist.hp = 0;
    this->statlist.spcAtk = 0;
    this->statlist.spcDef = 0;
}

// todo(snail)
//  maybe I should move this out into an abstraction so its easier to change
void Creature::load(CreatureData_t seed) {
    this->id = static_cast<uint8_t>((seed.id));
    this->loadTypes(seed);
    this->setStats(seed);
    // Need some kind of default setting for moves ?
    this->loadMoves(seed);
    // this->loadSprite(seed);
}

// 00,id1,lvl1,move11,move12,move13,move14,
void Creature::loadFromOpponentSeed(uint32_t seed) {
    CreatureData_t cSeed = getCreatureFromStore(seed);
    this->id = static_cast<uint8_t>((cSeed.id));
    this->loadTypes(cSeed);
    this->level = parseOpponentCreatureSeedlvl(seed);
    this->setStats(cSeed);
    this->setMove(parseOpponentCreatureSeedMove(seed, 0), 0);
    this->setMove(parseOpponentCreatureSeedMove(seed, 1), 1);
    this->setMove(parseOpponentCreatureSeedMove(seed, 2), 2);
    this->setMove(parseOpponentCreatureSeedMove(seed, 3), 3);
    // this->loadSprite(cSeed);
}

void Creature::arenaLoad(uint24_t addr, uint8_t lvl) {
    uint8_t data[4];
    data[0] = FX::readIndexedUInt8(addr, 1);
    data[1] = FX::readIndexedUInt8(addr, 2);
    data[2] = FX::readIndexedUInt8(addr, 3);
    data[3] = FX::readIndexedUInt8(addr, 4);

    this->id = FX::readIndexedUInt8(addr, 0);
    CreatureData_t cSeed = getCreatureFromStore(id);

    this->loadTypes(cSeed);
    this->level = lvl;
    this->setStats(cSeed);
    this->setMove(FX::readIndexedUInt8(addr, 1), 0);
    this->setMove(FX::readIndexedUInt8(addr, 2), 1);
    this->setMove(FX::readIndexedUInt8(addr, 3), 2);
    this->setMove(FX::readIndexedUInt8(addr, 4), 3);
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
    return seed;
    // return (2*this->level)*(seed/3);
}

// TODO deal with drawing numbers
void Creature::printCreature() {
    // Arduboy2::fillRect(0, 0, 128, 66, WHITE);
    //  Arduboy2::drawRect(2, 1, 124, 43, BLACK);
    SpritesU::fillRect(0, 0, 128, 60, WHITE);
    FX::setCursor(34, 3);
    printString(font, MenuFXData::hpText, 4, 3);
    FX::drawNumber(this->statlist.hp);
    FX::setCursor(90, 3);
    printString(font, MenuFXData::atkText, 60, 3);
    FX::drawNumber(this->statlist.attack);
    FX::setCursor(34, 11);
    printString(font, MenuFXData::defText, 4, 11);
    FX::drawNumber(this->statlist.defense);
    FX::setCursor(90, 11);
    printString(font, MenuFXData::spdText, 60, 11);
    FX::drawNumber(this->statlist.speed);
    FX::setCursor(34, 19);
    printString(font, MenuFXData::satkText, 4, 19);
    FX::drawNumber(this->statlist.spcAtk);
    FX::setCursor(90, 19);
    printString(font, MenuFXData::sdefText, 60, 19);
    FX::drawNumber(this->statlist.spcDef);
    for (uint8_t i = 0; i < 4; i++) {
        if (this->moves[i] == 32) {
            continue;
        }
        uint8_t offset = i % 2;
        // FX::setCursor(4 + (60 * offset), 27 + (8 * (i / 2)));
        uint24_t rowAddress = FX::readIndexedUInt24(MoveData::moveNames, this->moves[i]);
        printString(font, rowAddress, 4 + (60 * offset), 27 + (8 * (i / 2)));
    }
}