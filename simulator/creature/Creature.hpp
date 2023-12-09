#pragma once
#include "../data/creatures.hpp"
#include "../data/teams.hpp"
#include "../lib/Move.hpp"
#include "../lib/Type.hpp"
#include "../opponent/OpponentSeed.hpp"
#include <stdint.h>

typedef struct stats {
    uint8_t attack;
    uint8_t defense;
    uint8_t hp;
    uint8_t speed;
    uint8_t spcAtk;
    uint8_t spcDef;
} stats_t;

class Creature {

  public:
    // potential saving: read this from progmem when need it ?
    // 6 instances to 1 stack var
    // uint64_t seed;
    // CreatureData_t* seed;
    // todo(snail)
    // need to squash these down into 1 byte -> 10 byte loss to 2byte loss
    // if I squash these i can use the 5thbit as a flag to show the second type is
    // none
    // or just have the two types be the same
    // if therye the same there's going to be issues with the modifer double
    // counting the
    uint8_t id;
    DualType types;
    uint8_t level;

    // Moves are held as an index to the move array
    uint8_t moves[4];
    Move moveList[4];
    stats_t statlist;

    // const unsigned char *sprite;

    Creature();
    void simuLoad(CreatureMoveset data);

    void setStats(CreatureData_t seed);
    void loadMoves(CreatureData_t seed);
    // void loadSprite(CreatureData_t seed);
    void loadTypes(CreatureData_t seed);
    void setMove(uint8_t move, uint8_t slot);

    uint8_t getAdvantage(DualType opponent);   // finds best advantage

    uint8_t getMove(uint8_t slot);

    uint8_t seedToStat(uint8_t seed);
    bool moveTypeBonus(uint8_t index);

    void printCreature();

#ifdef CLI
    void printMoves();
    void printStats();
#endif
};
