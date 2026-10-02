#include "ReadData.hpp"
#include <ArduboyFX.h>
#include <avr/pgmspace.h>
#include "../fxdata.h"

namespace {
// Character counts in data/text/strings.txt. The legacy string parser keeps
// one leading space, so generated bitmaps are five pixels wider than the text.
const uint8_t creatureNameLengths[] PROGMEM = {
    13, 12, 13, 11, 11, 11, 5, 8, 12, 4, 5, 8, 5, 5, 4, 10,
    10, 9, 7, 6, 4, 4, 5, 3, 3, 8, 6, 10, 7, 7, 5, 4,
};
const uint8_t moveNameLengths[] PROGMEM = {
    5, 7, 6, 5, 6, 6, 4, 7, 6, 6, 7, 4, 8, 8, 8, 7, 4,
    4, 5, 5, 4, 6, 4, 3, 6, 10, 8, 4, 4, 9, 4, 9, 0,
};

uint8_t bitmapWidth(const uint8_t *lengths, uint8_t count, uint16_t id) {
    if (id >= count) {
        return 0;
    }
    const uint8_t characters = pgm_read_byte(lengths + id);
    return characters == 0 ? 0 : static_cast<uint8_t>(5 * (characters + 1));
}
} // namespace

uint8_t readCreatureNameWidth(uint8_t id) {
    return bitmapWidth(creatureNameLengths, sizeof(creatureNameLengths), id);
}

uint8_t readMoveNameWidth(uint16_t id) {
    return bitmapWidth(moveNameLengths, sizeof(moveNameLengths), id);
}

uint8_t readEffectStringWidth() {
    // EffectStrings contains only the "applied effect" entry.
    return 75;
}

uint24_t readCreatureNameAddress(uint8_t id) {
    return FX::readIndexedUInt24(CreatureNames::CreatureNames, id);
}

uint24_t readMoveNameAddress(uint16_t id) {
    return FX::readIndexedUInt24(MoveNames::MoveNames, id);
}

uint24_t readEffectStringAddress() {
    return FX::readIndexedUInt24(EffectStrings::EffectStrings, 0);
}

uint8_t getEffectRateFX(uint8_t id) {
    return 100;
}
Move readMoveFX(uint8_t index) {
    uint32_t buffer;
    Move move;
    auto offset = sizeof(uint32_t) * index;
    uint24_t rowAddress = move_table + offset;
    buffer = FX::readIndexedUInt32(rowAddress, 0);
    move = Move(buffer);
    return move;
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
void load(Creature *creature, CreatureData_t seed, uint8_t level) {
    creature->id = static_cast<uint8_t>((seed.id));
    creature->level = level;
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
    opt->levels[0] = level;
    opt->levels[1] = 0;
    opt->levels[2] = 0;
    load(&opt->party[0], cseed, level);
    //  this->party[1].load(eseed);
    //  this->party[2].load(eseed);
}

uint16_t ReadFXu16(uint24_t addr) {
    FX::seekData(addr);
    uint8_t bytes[2];
    FX::readBytes(bytes, 2);
    FX::readEnd();
    return static_cast<uint16_t>(bytes[1]) << 8 | bytes[0];
}
