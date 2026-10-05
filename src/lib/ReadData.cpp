#include "ReadData.hpp"
#include "FxRead.hpp"
#include "MoveIds.hpp"
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
    7, 4, 0, 7, 7, 7, 5, 9, 8, 11, 10, 6,
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
    return FxRead::indexed24(CreatureNames::CreatureNames, id);
}

uint24_t readMoveNameAddress(uint16_t id) {
    return FxRead::indexed24(MoveNames::MoveNames, id);
}

uint24_t readEffectStringAddress() {
    return FxRead::indexed24(EffectStrings::EffectStrings, 0);
}

uint8_t getEffectRateFX(uint8_t id) {
    return 100;
}
Move readMoveFX(uint8_t index) {
    index = moveRecordIndex(index);
    if (index == EMPTY_MOVE_ID) return Move();
    uint32_t buffer;
    Move move;
    auto offset = sizeof(uint32_t) * index;
    uint24_t rowAddress = move_table + offset;
    buffer = FxRead::indexed32(rowAddress, 0);
    move = Move(buffer);
    return move;
}

OpponentSeed readOpponentSeed(uint8_t index) {
    OpponentSeed seed = OpponentSeed{0, 0, 1};
    uint24_t rowAddress = FxRead::indexed24(opts, index);
    FxRead::object(rowAddress, seed);
    return seed;
}

CreatureData_t getCreatureFromStore(uint8_t id) {
    CreatureData_t cseed;
    uint24_t rowAddress = CreatureData::creatureData + (sizeof(CreatureData_t) * id);
    FxRead::object(rowAddress, cseed);
    return cseed;
}

// 00,id1,lvl1,move11,move12,move13,move14,

void arenaLoad(Creature *creature, uint24_t addr, uint8_t lvl) {
    uint8_t record[5];
    FxRead::bytes(addr, record, sizeof(record));
    creature->id = record[0];
    CreatureData_t cSeed = getCreatureFromStore(creature->id);

    creature->loadTypes(cSeed);
    creature->level = lvl;
    creature->setStats(cSeed);
    for (uint8_t slot = 0; slot < 4; ++slot) {
        creature->setMove(record[slot + 1], slot);
    }
    // One record block, one creature seed, and the four packed move records
    // resolved by Creature::setMove() form this load transition.
    uint8_t reads = 2;
    for (uint8_t slot = 0; slot < 4; ++slot)
        if (validMoveId(record[slot + 1])) ++reads;
    FxReadCounter::transitionExact(reads);
}

uint16_t ReadFXu16(uint24_t addr) {
    return FxRead::littleEndian16(addr);
}
