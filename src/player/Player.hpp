#pragma once
#include <stdint.h>

#include "../creature/Creature.hpp"
#include "../lib/DataTypes.hpp"
#include "../item/Inventory.hpp"
#include "../item/KeyItems.hpp"

class Player {
    struct caughtCreature {
        uint8_t id;
        uint8_t level;
    };

  public:
    Creature party[3];
    uint8_t creatureHPs[3];
    // Single Player-owned bag. World deposits, the material picker and the v3
    // snapshot all read this one instance; there is no second inventory.
    item::Inventory inventory;
    item::KeyItems keyItems;

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