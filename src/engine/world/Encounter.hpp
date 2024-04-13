#pragma once
#include <ArduboyFX.h>
#include <stdint.h>

typedef struct EncounterTable {
    unsigned char c1;
    unsigned char c2;
    unsigned char c3;
    unsigned char c4;
    // 4bit encounter rates
    unsigned char rate1;
    unsigned char rate2;
    unsigned char rate3;
    unsigned char rate4;
    unsigned char avgLevel;

} EncounterTable_t;

class Encounter {
  private:
    uint8_t table[10];
    uint8_t avgLevel;
    Arduboy2Base *arduboy;

  public:
    void loadEncounterTable(uint8_t areaIndex);
    // returns creatureID based on available encounters and rates of current
    // EncounterTable
    uint8_t rollEncounter();
    uint8_t rollLevel();
};
