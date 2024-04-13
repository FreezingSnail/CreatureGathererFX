#include "Encounter.hpp"
#include <ArduboyFX.h>
#include <stdint.h>

#include "../../fxdata.h"

void __attribute__((optimize("-O0"))) Encounter::loadEncounterTable(uint8_t areaIndex) {
    EncounterTable_t table;
    uint24_t rowAddress = encounterRates + (sizeof(EncounterTable_t) * areaIndex);
    FX::readDataObject(rowAddress, table);

    for (uint8_t i = 0; i < table.rate1; i++) {
        this->table[i] = table.c1;
    }
    for (uint8_t i = 0; i < table.rate2; i++) {
        this->table[i + table.rate1] = table.c2;
    }
    for (uint8_t i = 0; i < table.rate3; i++) {
        this->table[i + table.rate1 + table.rate2] = table.c3;
    }
    for (uint8_t i = 0; i < table.rate4; i++) {
        this->table[i + table.rate1 + table.rate2 + table.rate3] = table.c4;
    }

    this->avgLevel = table.avgLevel;
}

uint8_t Encounter::rollEncounter() {
    uint8_t roll = random(0, 10);
    return this->table[roll];
}

uint8_t Encounter::rollLevel() {
    int8_t roll = random(0, 7) - 4;
    return this->avgLevel + roll;
}
