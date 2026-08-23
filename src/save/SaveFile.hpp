#pragma once

#include <stddef.h>
#include <stdint.h>

#include "../creature/Creature.hpp"
#include "../flags/flag_bit_array.hpp"
#include "../plants/PlantGamestate.hpp"
#include "../player/Player.hpp"

constexpr uint8_t SAVE_VERSION = 1;
constexpr uint8_t PARTY_MAX = sizeof(((Player *)nullptr)->party) / sizeof(Creature);
constexpr uint8_t FLAG_BYTES = sizeof(FLAG_BIT_ARRAY);
constexpr uint8_t INVENTORY_BYTES = sizeof(((Player *)nullptr)->items);

struct PlantSaveBlock {
    PlantStage plantStages;
    PlantPair plantPairs[2];
    uint8_t ticker;
};

struct SaveFile {
    uint8_t version;
    uint8_t reserved;
    uint16_t playerLocation;
    uint8_t flags[FLAG_BYTES];
    Creature party[PARTY_MAX];
    PlantSaveBlock plants;
    uint8_t inventory[INVENTORY_BYTES];
    uint16_t checksum;
};

static_assert(sizeof(SaveFile) + 2 <= 4094,
              "SaveFile must fit sector 0 with room to append");

// On failure, leaves out unchanged so callers retain their live/new-game state.
bool saveFileLoad(SaveFile &out);
void saveFileCommit(const SaveFile &in);
uint16_t saveFileChecksum(const SaveFile &in);
