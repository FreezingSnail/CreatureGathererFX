#pragma once

#include "../lib/DataTypes.hpp"
#include "../lib/Type.hpp"
#include "../lib/Stats.hpp"
#include "../lib/StatusEffect.hpp"
#include "../lib/StatModifier.hpp"
#include "../save/StoreRecord.hpp"

class Creature {

  public:
    uint8_t id;
    DualType types;
    uint8_t level;

    // Folded 5-bit duel attack set; bit i = duel type i, 0 = WAIT.
    uint8_t attackMask;
    // Maximum HP: 2*level + hpSeed*(level/3) + 30. The duel reads only HP.
    uint8_t hp;
    StatusEffect status;
    StatModifer statMods;

    Creature();
    void load(CreatureData_t seed);
    void load(const StoreRecord &record);
    void loadFromOpponentSeed(CreatureSeed seed);

    void setStats(CreatureData_t seed);
    // void loadSprite(CreatureData_t seed);
    void loadTypes(CreatureData_t seed);

    uint8_t getAdvantage(DualType opponent);   // finds best advantage

    uint8_t seedToStat(uint8_t seed);
};
