#pragma once

#include <stdint.h>

#include "../../creature/Creature.hpp"
#include "../../values.hpp"

namespace Encounter {

constexpr uint8_t SLOT_COUNT = 10;
constexpr uint8_t TABLE_BYTES = SLOT_COUNT + 2;
constexpr uint8_t ZONE_BYTES = 10;
constexpr uint8_t ZONE_COUNT = 8;
// Zone 0 owns the pocket's lure-biased group: one base wild table plus four
// lure tables. Selection bounds every lure offset against this count.
constexpr uint8_t TABLE_COUNT = 5;
constexpr uint8_t NIBBLE_COUNT = 4;

// Lure table offset meaning "no active lure"; select() then uses the cached
// base table. Mirrors LureZone::INERT_OFFSET without coupling the two headers.
constexpr uint8_t INERT_OFFSET = 0xFF;

constexpr uint8_t CACHE_VALID_OFFSET = TABLE_BYTES;
constexpr uint8_t CACHE_CHUNK_LOW_OFFSET = CACHE_VALID_OFFSET + 1;
constexpr uint8_t CACHE_CHUNK_HIGH_OFFSET = CACHE_VALID_OFFSET + 2;
constexpr uint8_t CACHE_TABLE_BASE_OFFSET = CACHE_VALID_OFFSET + 3;
constexpr uint8_t CACHE_TABLE_COUNT_OFFSET = CACHE_VALID_OFFSET + 4;
constexpr uint8_t CACHE_TYPE_MASK_OFFSET = CACHE_VALID_OFFSET + 5;
constexpr uint8_t CACHE_LEVEL_OFFSET = CACHE_VALID_OFFSET + 6;
constexpr uint8_t CACHE_TABLE_INDEX_OFFSET = CACHE_VALID_OFFSET + 7;
// Two bytes pack the zone's four lure-slot nibbles read at zone load.
constexpr uint8_t CACHE_NIBBLES_OFFSET = CACHE_TABLE_INDEX_OFFSET + 1;
constexpr uint8_t CACHE_BYTES = CACHE_NIBBLES_OFFSET + 2;

using Rng = uint8_t (*)(uint8_t lower, uint8_t upper);

struct Decision {
    uint8_t creatureId;
    uint8_t level;
    uint8_t slot;
};

// Cache one zone's base wild table at load, a chunk-change transition. select()
// reuses the cached base table with no FX reads; a lure-biased pick fetches its
// table once at battle entry, keeping the inert step path RAM-only.
bool load(uint8_t *cache, uint16_t chunkId);
void invalidate(uint8_t *cache);

// Select exactly one of the ten table slots and apply the cached level rule.
// `lureTableOffset` is LureZone::tableOffset(): a lure type (0..3) selects a
// lure-biased table through the cached nibbles, while INERT_OFFSET keeps the
// base table. Authored/packed validation bounds every table span, species ID,
// level bound and lure nibble for shipping content, so selection re-checks only
// the cache-valid state, the drawn slot and a nonempty party.
// Empty party slots contribute level zero while the divisor remains PARTY_SIZE.
bool select(const uint8_t *cache, const Creature party[PARTY_SIZE], Rng rng,
            uint8_t lureTableOffset, Decision &out);
uint8_t partyAverageLevel(const Creature party[PARTY_SIZE]);

#ifdef TEST
void setRngForTest(Rng rng);
#endif

} // namespace Encounter

static_assert(Encounter::CACHE_BYTES == 22,
              "encounter cache must fit the world transient budget");
