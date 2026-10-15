#pragma once
#include <stdint.h>

#include "../creature/Creature.hpp"
#include "../lib/DataTypes.hpp"
#include "../item/Item.hpp"

class Player {
    struct caughtCreature {
        uint8_t id;
        uint8_t level;
    };

  public:
    Creature party[3];
    uint8_t creatureHPs[3];
    Item items[10];

    // caughtCreature storedCreatures[6];

    Player();
    void basic();
    void loadCreature(uint8_t index, uint8_t creatureIndex);
    void setCreature(uint8_t index, CreatureData_t seed);
    // Restore every occupied party member to its derived maximum HP and clear
    // transient status. Empty slots stay empty and boxed instances are never
    // touched. Returns whether any live HP changed.
    bool healParty();
    void restore(const Creature *savedParty, const uint8_t *savedHP);
    void storeCreature(uint8_t slot, uint8_t id, uint8_t level);
};