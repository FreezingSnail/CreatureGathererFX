#pragma once
#include <stdint.h>

typedef struct OpponentSeed {

    // id,lvl,move4,move3,move2,move1
    // 00000 00000 00000 00000 00000 00000
    uint32_t firstCreature;
    uint32_t secondCreature;
    uint32_t thirdCreature;

} OpponentSeed_t;

// id1,lvl1,move11,move12,move13,move14,
inline uint8_t parseOpponentCreatureSeedlvl(uint32_t seed) {
    return (seed & 0b00000001111100000000000000000000) >> 20;
}

inline uint8_t parseOpponentCreatureSeedID(uint32_t seed) {
    return (seed & 0b00111110000000000000000000000000) >> 25;
}

inline uint8_t parseOpponentCreatureSeedMove(uint32_t seed, uint8_t move) {
    return ((seed & (0b00011111 << (5 * move))) >> 25);
}

// todo research huffman encoding to squash these in mem
typedef struct CreatureD {
    unsigned char id;
    unsigned char type1;
    unsigned char type2;
    unsigned char evoLevel;
    unsigned char atkSeed;
    unsigned char defSeed;
    unsigned char spcAtkSeed;
    unsigned char spcDefSeed;
    unsigned char hpSeed;
    unsigned char spdSeed;
    unsigned char move1;
    unsigned char move2;
    unsigned char move3;
    unsigned char move4;
} CreatureData_t;