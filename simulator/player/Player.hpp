#pragma once
#include <stdint.h>

#include "../creature/Creature.hpp"
#include "../data/creatures.hpp"

class Player {
    struct caughtCreature {
        uint8_t id;
        uint8_t level;
    };

  public:
    Creature party[3];
    uint8_t creatureHPs[3];
    uint8_t teamID;

    void Read(uint8_t index);
};