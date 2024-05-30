#pragma once
#include <stdint.h>
#include "MenuData.hpp"

// id,lvl,move4,move3,move2,move1
// 00000 00000 00000 00000 00000 00000
typedef struct OpponentSeed {
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
    uint8_t id;
    uint8_t type1;
    uint8_t type2;
    uint8_t evoLevel;
    uint8_t atkSeed;
    uint8_t defSeed;
    uint8_t spcAtkSeed;
    uint8_t spcDefSeed;
    uint8_t hpSeed;
    uint8_t spdSeed;
    uint8_t move1;
    uint8_t move2;
    uint8_t move3;
    uint8_t move4;
} CreatureData_t;

struct PopUpDialog {
    uint8_t x, y, width, height;
    uint24_t textAddress;
    uint16_t damage;
    DialogType type;
    uint24_t animation;
};