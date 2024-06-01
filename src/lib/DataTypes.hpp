#pragma once
#include <stdint.h>
#include "MenuData.hpp"

// id,lvl,move4,move3,move2,move1
// 00000 00000 00000 00000 00000 00000

struct CreatureSeed {
    uint8_t id;
    uint8_t lvl;
    uint32_t moves;
};

struct OpponentSeed {
    CreatureSeed firstCreature;
    CreatureSeed secondCreature;
    CreatureSeed thirdCreature;
};

inline uint8_t parseOpponentCreatureSeedMove(uint32_t seed, uint8_t move) {
    uint8_t shift = 8 * move;
    return ((seed & (0b11111111 << (shift))) >> shift);
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