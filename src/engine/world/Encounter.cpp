#include "Encounter.hpp"

#include "../../lib/FxRead.hpp"
#include "../../lib/random.hpp"
#include "../../fxdata.h"

namespace Encounter {
namespace {

constexpr uint8_t SPECIES_COUNT = 32;
#ifdef TEST
Rng testRng = nullptr;
#endif

uint16_t readChunkId(const uint8_t *zone) {
    return static_cast<uint16_t>(zone[0]) |
           (static_cast<uint16_t>(zone[1]) << 8);
}

// Four lure-slot nibbles live in two cache bytes: index 0/2 in the low nibble,
// index 1/3 in the high nibble.
uint8_t lureNibble(const uint8_t *cache, uint8_t index) {
    const uint8_t packed = cache[CACHE_NIBBLES_OFFSET + (index >> 1)];
    return (index & 1) ? static_cast<uint8_t>(packed >> 4)
                       : static_cast<uint8_t>(packed & 0x0F);
}

bool validTable(const uint8_t *table) {
    if (table[SLOT_COUNT] == 0 || table[SLOT_COUNT] > table[SLOT_COUNT + 1]) {
        return false;
    }
    for (uint8_t slot = 0; slot < SLOT_COUNT; ++slot) {
        if (table[slot] >= SPECIES_COUNT) return false;
    }
    return true;
}

Rng selectedRng(Rng requested) {
#ifdef TEST
    return requested != nullptr ? requested : testRng;
#else
    return requested;
#endif
}

uint8_t rollSlot(Rng rng) {
    return rng == nullptr ? randomRoll(0, SLOT_COUNT - 1)
                           : rng(0, SLOT_COUNT - 1);
}

} // namespace

void invalidate(uint8_t *cache) {
    for (uint8_t i = 0; i < CACHE_BYTES; ++i) cache[i] = 0;
}

bool load(uint8_t *cache, uint16_t chunkId) {
    invalidate(cache);
    uint8_t reads = 0;

    for (uint8_t zoneIndex = 0; zoneIndex < ZONE_COUNT; ++zoneIndex) {
        const uint24_t zoneAddress = EncounterFXData::zoneDefs +
            static_cast<uint24_t>(zoneIndex) * ZONE_BYTES;
        FxRead::bytes(zoneAddress, cache, ZONE_BYTES);
        ++reads;
        if (readChunkId(cache) != chunkId) continue;

        const uint8_t tableBase = cache[2];
        const uint8_t tableCount = cache[3];
        const uint8_t typeMask = cache[4];
        const uint8_t levelOffset = cache[9];
        // Capture the lure nibbles before the table read overwrites the zone
        // def payload at cache[0..TABLE_BYTES-1].
        const uint8_t nibble0 = cache[5];
        const uint8_t nibble1 = cache[6];
        const uint8_t nibble2 = cache[7];
        const uint8_t nibble3 = cache[8];
        if (tableCount == 0 || tableBase >= TABLE_COUNT ||
            static_cast<uint16_t>(tableBase) + tableCount > TABLE_COUNT) {
            FxReadCounter::transitionExact(reads);
            invalidate(cache);
            return false;
        }

        const uint24_t tableAddress = EncounterFXData::tables +
            static_cast<uint24_t>(tableBase) * TABLE_BYTES;
        FxRead::bytes(tableAddress, cache, TABLE_BYTES);
        ++reads;
        if (!validTable(cache)) {
            FxReadCounter::transitionExact(reads);
            invalidate(cache);
            return false;
        }

        // Keep transition metadata after the table payload. The nibbles drive
        // lure-biased table selection in select(); tableBase is the wild table.
        cache[CACHE_VALID_OFFSET] = 1;
        cache[CACHE_CHUNK_LOW_OFFSET] = static_cast<uint8_t>(chunkId);
        cache[CACHE_CHUNK_HIGH_OFFSET] = static_cast<uint8_t>(chunkId >> 8);
        cache[CACHE_TABLE_BASE_OFFSET] = tableBase;
        cache[CACHE_TABLE_COUNT_OFFSET] = tableCount;
        cache[CACHE_TYPE_MASK_OFFSET] = typeMask;
        cache[CACHE_LEVEL_OFFSET] = levelOffset;
        cache[CACHE_TABLE_INDEX_OFFSET] = tableBase;
        cache[CACHE_NIBBLES_OFFSET] = static_cast<uint8_t>(
            (nibble0 & 0x0F) | ((nibble1 & 0x0F) << 4));
        cache[CACHE_NIBBLES_OFFSET + 1] = static_cast<uint8_t>(
            (nibble2 & 0x0F) | ((nibble3 & 0x0F) << 4));
        FxReadCounter::transitionExact(reads);
        return true;
    }

    invalidate(cache);
    FxReadCounter::transitionExact(reads);
    return false;
}

uint8_t partyAverageLevel(const Creature party[PARTY_SIZE]) {
    uint16_t total = 0;
    for (uint8_t slot = 0; slot < PARTY_SIZE; ++slot) {
        // A zero level is the explicit uninitialized-slot sentinel. Species 0
        // remains a valid party member, so ID cannot be used as the sentinel.
        total = static_cast<uint16_t>(total + party[slot].level);
    }
    return static_cast<uint8_t>(total / PARTY_SIZE);
}

bool select(const uint8_t *cache, const Creature party[PARTY_SIZE], Rng rng,
            uint8_t lureTableOffset, Decision &out) {
    if (cache[CACHE_VALID_OFFSET] != 1) return false;

    const uint8_t tableBase = cache[CACHE_TABLE_BASE_OFFSET];
    uint8_t tableIndex = tableBase;
    if (lureTableOffset != INERT_OFFSET) {
        tableIndex = static_cast<uint8_t>(
            tableBase + lureNibble(cache, lureTableOffset & 0x03u));
    }

    // The cache holds the base table; a lure-biased pick fetches its table
    // once at battle entry. The inert path stays RAM-only. Authored and packed
    // validation prove every table span, species ID, level bound and lure
    // nibble, so the cache-valid state, the drawn slot and a nonempty party are
    // the only selection-time concerns left.
    uint8_t lureTable[TABLE_BYTES];
    const uint8_t *table = cache;
    if (tableIndex != cache[CACHE_TABLE_INDEX_OFFSET]) {
        const uint24_t address = EncounterFXData::tables +
            static_cast<uint24_t>(tableIndex) * TABLE_BYTES;
        FxRead::bytes(address, lureTable, TABLE_BYTES);
        table = lureTable;
    }

    bool hasParty = false;
    for (uint8_t slot = 0; slot < PARTY_SIZE; ++slot) {
        if (party[slot].level != 0) {
            hasParty = true;
            break;
        }
    }
    if (!hasParty) return false;

    const uint8_t slot = rollSlot(selectedRng(rng));
    if (slot >= SLOT_COUNT || table[slot] >= SPECIES_COUNT) return false;

    const int16_t rawLevel = static_cast<int16_t>(partyAverageLevel(party)) +
        static_cast<int8_t>(cache[CACHE_LEVEL_OFFSET]);
    const int16_t floor = table[TABLE_BYTES - 2];
    const int16_t ceil = table[TABLE_BYTES - 1];
    int16_t level = rawLevel;
    if (level < floor) level = floor;
    if (level > ceil) level = ceil;
    if (level <= 0 || level > 255) return false;

    out.creatureId = table[slot];
    out.level = static_cast<uint8_t>(level);
    out.slot = slot;
    return true;
}

#ifdef TEST
void setRngForTest(Rng rng) {
    testRng = rng;
}
#endif

} // namespace Encounter
