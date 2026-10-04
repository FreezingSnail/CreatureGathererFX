#include "../../src/lib/ReadData.hpp"
#include "../../src/lib/FxRead.hpp"
#include "parseCSV.hpp"

uint24_t readCreatureNameAddress(uint8_t id) {
    FxReadCounter::record();
    return 0x10000UL + id;
}

uint24_t readMoveNameAddress(uint16_t id) {
    FxReadCounter::record();
    return 0x20000UL + id;
}

uint24_t readEffectStringAddress() {
    return 0x30000UL;
}

uint8_t readCreatureNameWidth(uint8_t id) {
    return id == 3 ? 60 : (id < 32 ? 70 : 0);
}

uint8_t readMoveNameWidth(uint16_t id) {
    return id < 32 ? 40 : 0;
}

uint8_t readEffectStringWidth() {
    return 75;
}

Move readMoveFX(uint8_t index) {
    FxReadCounter::record();
    Move move;
    // uint24_t rowAddress = MoveData::movePack + sizeof(MoveBitSet) * index;
    // FX::readDataObject(rowAddress, move);
    return move;
}

uint8_t getEffectRateFX(uint8_t id) {
    return 100;
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
    OpponentSeed seed;
    std::string line = readLineFromCSV(OPTCSV, index);
    auto opponent = parseOpponentCSVLine(line);
    seed = convertToOpponentSeed(opponent);
    return seed;
}

CreatureData_t getCreatureFromStore(uint8_t id) {
    CreatureData_t cseed;
    std::string line = readLineFromCSV(CREATURECSV, id);
    CSVCreature csvCreature = parseCSVLineToCreature(line);
    // printCSVCreature(csvCreature);
    cseed = CSVCreatureConvert(csvCreature);
    return cseed;
}

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
}
