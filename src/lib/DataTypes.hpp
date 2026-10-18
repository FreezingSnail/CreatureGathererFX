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
    // Authored HP seed; max HP = 2*level + hpSeed*(level/3) + 30. The five
    // dead stat seeds were retired (CreatureGathererFX-rhs7.3): the duel reads
    // only HP. Field order is a permanent packed ABI.
    uint8_t hpSeed;
    // Folded 5-bit duel attack set; 0 = no usable attack (WAIT).
    uint8_t attackMask;
} CreatureData_t;

#ifdef __AVR__
static_assert(sizeof(CreatureData_t) == 6, "creature record is 6 bytes");
#endif

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
