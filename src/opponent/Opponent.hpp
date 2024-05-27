#pragma once
#include "../creature/Creature.hpp"
#include "../lib/DataTypes.hpp"
#include "../lib/uint24.h"

// This class is a non-functional datapack to load into the battle engine on an
// encounter Opponets should be stored in PROGMEM as a list of 3 pointers
// signifying the part and a list of levels

class Opponent {
  public:
    Creature party[3];
    uint8_t levels[3];
    uint24_t nameptr;

    // This should load information from PROGMEM
    Opponent();
    void loadOpt(OpponentSeed_t *seed);

    void loadCreature(OpponentSeed_t *seed);
};