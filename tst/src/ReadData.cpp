#include "../../src/lib/ReadData.hpp"
#include "parseCSV.hpp"

MoveBitSet getMovePack(uint8_t index) {
    MoveBitSet move;
    // uint24_t rowAddress = MoveData::movePack + sizeof(MoveBitSet) * index;
    // FX::readDataObject(rowAddress, move);
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
    uint8_t data[4];
    // data[0] = FX::readIndexedUInt8(addr, 1);
    // data[1] = FX::readIndexedUInt8(addr, 2);
    // data[2] = FX::readIndexedUInt8(addr, 3);
    // data[3] = FX::readIndexedUInt8(addr, 4);

    // creature->id = FX::readIndexedUInt8(addr, 0);
    CreatureData_t cSeed = getCreatureFromStore(creature->id);

    creature->loadTypes(cSeed);
    creature->level = lvl;
    creature->setStats(cSeed);
    // creature->setMove(FX::readIndexedUInt8(addr, 1), 0);
    // creature->setMove(FX::readIndexedUInt8(addr, 2), 1);
    // creature->setMove(FX::readIndexedUInt8(addr, 3), 2);
    // creature->setMove(FX::readIndexedUInt8(addr, 4), 3);
}

void ReadOpt(Opponent *opt, uint8_t index) {
    uint24_t creatures[3];
    // uint24_t addr = Teams::teamList + sizeof(uint24_t) * 7 * index;
    // creatures[0] = FX::readIndexedUInt24(addr, 0);
    // creatures[1] = FX::readIndexedUInt24(addr, 1);
    // creatures[2] = FX::readIndexedUInt24(addr, 2);
    // opt->levels[0] = uint8_t(FX::readIndexedUInt24(addr, 3));
    // opt->levels[1] = uint8_t(FX::readIndexedUInt24(addr, 4));
    // opt->levels[2] = uint8_t(FX::readIndexedUInt24(addr, 5));
    arenaLoad(&opt->party[0], creatures[0], opt->levels[0]);
    arenaLoad(&opt->party[0], creatures[1], opt->levels[1]);
    arenaLoad(&opt->party[0], creatures[2], opt->levels[2]);

    // opt->nameptr = FX::readIndexedUInt24(addr, 6);
}

void loadEncounterOpt(Opponent *opt, uint8_t id, uint8_t level) {
    CreatureData_t cseed;
    // uint24_t rowAddress = CreatureData::creatureData + (sizeof(CreatureData_t) * id);
    // FX::readDataObject(rowAddress, cseed);
    // opt->levels[0] = 2;
    // opt->levels[1] = 0;
    // opt->levels[2] = 0;
    // load(&opt->party[0], cseed);
    //  this->party[1].load(eseed);
    //  this->party[2].load(eseed);
}