#include "Encounter.hpp"

#include <stdint.h>

#include "../../data/Encounters.hpp"
#include "Arduboy2.h"

Encounter::Encounter() { this->arduboy = nullptr; }

Encounter::Encounter(Arduboy2* arduboy) {
  this->arduboy = arduboy;
  this->loadEncounterTable(0);
}

void Encounter::loadEncounterTable(int areaIndex) {
  EncounterTable_t table;
  memcpy_P(&table, &encounterRates[areaIndex], sizeof(EncounterTable_t));
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
