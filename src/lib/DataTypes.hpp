#pragma once
#include <stdint.h>
#include "MenuData.hpp"

// id,lvl,attackMask  (bit i = duel type i; 0 = WAIT)
struct CreatureSeed {
    uint8_t id;
    uint8_t lvl;
    uint8_t attackMask;
};

struct OpponentSeed {
    CreatureSeed firstCreature;
    CreatureSeed secondCreature;
    CreatureSeed thirdCreature;
};

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
    // Folded 5-bit duel attack set; 0 = no usable attack (WAIT).
    uint8_t attackMask;
} CreatureData_t;

struct PopUpDialog {
    uint8_t x, y;
    // TEXT keeps its raster dimensions; SCRIPT_TEXT uses width as the prepared
    // character count for the resident two-row text buffer.
    uint8_t width, height;
    // TEXT holds an event address; SCRIPT_TEXT holds a raw_map_text index.
    uint24_t textAddress;
    DialogType type;
};

static_assert(TEXT == 0 && SCRIPT_TEXT == 15, "dialog type IDs remain stable");
#ifdef __AVR__
static_assert(sizeof(PopUpDialog) == 8, "AVR dialog descriptor must remain 8 bytes");
#endif
