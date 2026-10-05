#include "../../src/lib/ReadData.hpp"
#include "../../src/lib/FxRead.hpp"
#include "../../src/lib/MoveIds.hpp"
#include <avr/pgmspace.h>
#include "parseCSV.hpp"
#include "../fxdatatest/generated/move_data.hpp"
#include "../fxdatatest/generated/opponent_data.hpp"

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
    index = moveRecordIndex(index);
    return index < moveFixtureCount ? Move(moveFixtures[index]) : Move();
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
    return index < opponentSeedCount ? opponentSeeds[index] : OpponentSeed{};
}

CreatureData_t getCreatureFromStore(uint8_t id) {
    CreatureData_t cseed;
    std::string line = readLineFromCSV(CREATURECSV, id);
    CSVCreature csvCreature = parseCSVLineToCreature(line);
    // printCSVCreature(csvCreature);
    cseed = CSVCreatureConvert(csvCreature);
    return cseed;
}
