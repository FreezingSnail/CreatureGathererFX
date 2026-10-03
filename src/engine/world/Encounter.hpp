#pragma once

#include <stdint.h>

#include "../../creature/Creature.hpp"
#include "../../values.hpp"

namespace Encounter {

constexpr uint8_t SLOT_COUNT = 10;
constexpr uint8_t TABLE_BYTES = SLOT_COUNT + 2;
constexpr uint8_t ZONE_BYTES = 10;
constexpr uint8_t ZONE_COUNT = 8;
constexpr uint8_t TABLE_COUNT = 1;

constexpr uint8_t CACHE_VALID_OFFSET = TABLE_BYTES;
constexpr uint8_t CACHE_CHUNK_LOW_OFFSET = CACHE_VALID_OFFSET + 1;
constexpr uint8_t CACHE_CHUNK_HIGH_OFFSET = CACHE_VALID_OFFSET + 2;
constexpr uint8_t CACHE_TABLE_BASE_OFFSET = CACHE_VALID_OFFSET + 3;
constexpr uint8_t CACHE_TABLE_COUNT_OFFSET = CACHE_VALID_OFFSET + 4;
constexpr uint8_t CACHE_TYPE_MASK_OFFSET = CACHE_VALID_OFFSET + 5;
constexpr uint8_t CACHE_LEVEL_OFFSET = CACHE_VALID_OFFSET + 6;
constexpr uint8_t CACHE_TABLE_INDEX_OFFSET = CACHE_VALID_OFFSET + 7;
constexpr uint8_t CACHE_BYTES = CACHE_TABLE_INDEX_OFFSET + 1;

using Rng = uint8_t (*)(uint8_t lower, uint8_t upper);

struct Decision {
    uint8_t creatureId;
    uint8_t level;
    uint8_t slot;
};

// Cache one zone's base wild table. The only FX reads happen inside this
// transition operation; select() is deliberately RAM-only for step dispatch.
bool load(uint8_t *cache, uint16_t chunkId);
void invalidate(uint8_t *cache);

// Select exactly one of the ten table slots and apply the cached level rule.
// Empty party slots contribute level zero while the divisor remains PARTY_SIZE.
bool select(const uint8_t *cache, const Creature party[PARTY_SIZE], Rng rng,
            Decision &out);
uint8_t partyAverageLevel(const Creature party[PARTY_SIZE]);

#ifdef TEST
void setRngForTest(Rng rng);
#endif

} // namespace Encounter

static_assert(Encounter::CACHE_BYTES == 20,
              "encounter cache must fit the world transient budget");
