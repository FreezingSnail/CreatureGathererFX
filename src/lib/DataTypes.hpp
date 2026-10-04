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
    return static_cast<uint8_t>(seed >> (8u * move));
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
    uint8_t x, y;
    // TEXT keeps its box dimensions. Indexed dialogs use width for the first
    // string bitmap and height for the move/effect bitmap; draw positions stay fixed.
    uint8_t width, height;
    // TEXT holds an event address; SCRIPT_TEXT holds a raw_map_text index;
    // name dialogs hold a resolved creature name. EFFECTIVENESS holds a Modifier.
    // Other types do not draw this field.
    uint24_t textAddress;
    // Resolved move name for NAME/ENEMY_NAME, or effect text for effect dialogs.
    uint24_t detailAddress;
    uint16_t damage;
    DialogType type;
    uint24_t animation;
};
