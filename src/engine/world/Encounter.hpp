#pragma once

#include <stdint.h>

#include "../../data/Encounters.hpp"
#include <ArduboyFX.h>

class Encounter {
private:
  uint8_t table[10];
  uint8_t avgLevel;
  Arduboy2 *arduboy;

public:
  Encounter();
  Encounter(Arduboy2 *arduboy);
  void loadEncounterTable(int areaIndex);
  // returns creatureID based on available encounters and rates of current
  // EncounterTable
  uint8_t rollEncounter();
  uint8_t rollLevel();
};
