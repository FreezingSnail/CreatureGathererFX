#pragma once
#include <stdint.h>
#include "../creature/Creature.hpp"
#include "../item/Item.hpp"

class Player {

    public:
        Creature party[3];
        uint8_t creatureHPs[3];
        Item items[10];

        Creature storedCreatures[10]; 


        Player();

        void setMonster(uint8_t index, CreatureSeed_t seed);
        
};