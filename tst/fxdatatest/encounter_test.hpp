#pragma once

#include "fxtest.hpp"
#include "src/engine/world/Encounter.hpp"
#include "src/engine/world/Chunk.hpp"
#include "src/fxdata.h"
#include "src/lib/FxReadCounter.hpp"

// Focused packed-device coverage for the validated wild-encounter path. It
// reads the real cart through Encounter::load/select and proves the runtime
// assumptions the authored/packed validator establishes: an authored zone
// caches its base table in one transition, the inert step path stays RAM-only,
// each live lure type fetches exactly one biased table at battle entry, and a
// missing zone or a wholly empty party refuses without rolling.
namespace encounterDeviceTest {
uint8_t rngCalls = 0;
uint8_t rngLower = 0;
uint8_t rngUpper = 0;

uint8_t chooseFirstSlot(uint8_t lower, uint8_t upper) {
    ++rngCalls;
    rngLower = lower;
    rngUpper = upper;
    return 0;
}
} // namespace encounterDeviceTest

inline void test_encounter(FxTest &test) {
    using namespace encounterDeviceTest;
    uint8_t cache[Encounter::CACHE_BYTES];
    Creature party[PARTY_SIZE] = {};
    for (uint8_t slot = 0; slot < PARTY_SIZE; ++slot) party[slot].level = 0;
    party[0].level = 9;
    Encounter::Decision decision = {};

    // Read expectations independently through the generated table addresses,
    // before starting the production metadata-read counter assertions.
    uint8_t zone[Encounter::ZONE_BYTES];
    FX::readDataBytes(EncounterFXData::zoneDefs, zone, sizeof(zone));
    const uint16_t chunk = static_cast<uint16_t>(zone[0]) |
        (static_cast<uint16_t>(zone[1]) << 8);
    uint8_t expected[Encounter::TABLE_BYTES];
    FX::readDataBytes(EncounterFXData::tables +
        static_cast<uint24_t>(zone[2]) * Encounter::TABLE_BYTES,
        expected, sizeof(expected));
    int16_t expectedLevel = party[0].level / PARTY_SIZE +
        static_cast<int8_t>(zone[9]);
    if (expectedLevel < expected[Encounter::SLOT_COUNT])
        expectedLevel = expected[Encounter::SLOT_COUNT];
    if (expectedLevel > expected[Encounter::SLOT_COUNT + 1])
        expectedLevel = expected[Encounter::SLOT_COUNT + 1];

    // The first authored zone caches its base table with two
    // transition reads: one zone definition plus one 12-byte table.
    FxReadCounter::resetFrame();
    const bool loaded = Encounter::load(cache, chunk);
    test.expectEq(loaded, true, F("published encounter zone loads"));
    test.expectEq(FxReadCounter::count(), static_cast<uint8_t>(2),
                  F("zone and base table read on load"));
    test.expectEq(FxReadCounter::transitionExact(0), true,
                  F("load owns the transition reads"));

    // Inert lure: the cached base table answers with no further FX read.
    rngCalls = 0;
    const bool selected = Encounter::select(cache, party, chooseFirstSlot,
                                            Encounter::INERT_OFFSET, decision);
    test.expectEq(selected, true, F("cached base table selects an encounter"));
    test.expectEq(decision.slot, static_cast<uint8_t>(0),
                  F("injected roll selects slot zero"));
    test.expectEq(decision.creatureId, expected[0],
                  F("base selection matches the published table"));
    test.expectEq(decision.creatureId, static_cast<uint8_t>(0),
                  F("species zero remains selectable"));
    test.expectEq(decision.level, static_cast<uint8_t>(expectedLevel),
                  F("party average uses the three-slot divisor"));
    test.expectEq(rngCalls, static_cast<uint8_t>(1), F("selection rolls once"));
    test.expectEq(rngLower, static_cast<uint8_t>(0), F("slot roll lower bound"));
    test.expectEq(rngUpper, static_cast<uint8_t>(Encounter::SLOT_COUNT - 1),
                  F("slot roll upper bound"));
    test.expectEq(FxReadCounter::count(), static_cast<uint8_t>(2),
                  F("inert selection reads only the RAM cache"));
    test.expectEq(FxReadCounter::markUpdate(), true,
                  F("inert step stays within the transition budget"));

    // Live lure types fetch their authored mapping at battle entry; a mapping
    // back to the base table remains RAM-only.
    for (uint8_t lure = 0; lure < Encounter::NIBBLE_COUNT; ++lure) {
        FX::readDataBytes(EncounterFXData::tables +
            static_cast<uint24_t>(zone[2] + zone[5 + lure]) * Encounter::TABLE_BYTES,
            expected, sizeof(expected));
        const uint8_t expectedReads = zone[5 + lure] == 0 ? 0 : 1;
        FxReadCounter::resetFrame();
        rngCalls = 0;
        const bool live = Encounter::select(cache, party, chooseFirstSlot, lure,
                                            decision);
        test.expectEq(live, true, F("active lure selects its biased table"));
        test.expectEq(decision.creatureId, expected[0],
                      F("lure type maps through its packed nibble"));
        test.expectEq(rngCalls, static_cast<uint8_t>(1), F("live lure rolls once"));
        test.expectEq(FxReadCounter::count(), expectedReads,
                      F("only the mapped lure table is read at battle entry"));
        test.expectEq(FxReadCounter::transitionExact(expectedReads), true,
                      F("lure table read belongs to the transition"));
        test.expectEq(FxReadCounter::markUpdate(), true,
                      F("lure entry leaves no per-frame reads"));
    }

    // Find a valid map chunk with no authored zone through the published table.
    uint16_t missingChunk = 0;
    bool found;
    do {
        found = false;
        for (uint8_t index = 0; index < Encounter::ZONE_COUNT; ++index) {
            uint8_t id[2];
            FX::readDataBytes(EncounterFXData::zoneDefs +
                static_cast<uint24_t>(index) * Encounter::ZONE_BYTES, id, sizeof(id));
            const uint16_t authored = static_cast<uint16_t>(id[0]) |
                (static_cast<uint16_t>(id[1]) << 8);
            if (authored == missingChunk) found = true;
        }
        if (found) ++missingChunk;
    } while (found);
    test.expectEq(Chunk::validChunkId(missingChunk), true,
                  F("missing zone is still a valid map chunk"));

    // A chunk with no authored zone scans only the definitions and
    // invalidates the cache, so selection refuses without rolling.
    FxReadCounter::resetFrame();
    const bool missing = Encounter::load(cache, missingChunk);
    test.expectEq(missing, false, F("missing authored chunk has no encounter"));
    test.expectEq(cache[Encounter::CACHE_VALID_OFFSET], static_cast<uint8_t>(0),
                  F("missing chunk invalidates the previous cache"));
    test.expectEq(FxReadCounter::count(),
                  static_cast<uint8_t>(Encounter::ZONE_COUNT),
                  F("missing chunk scans only zone definitions"));
    test.expectEq(FxReadCounter::transitionExact(0), true,
                  F("missed zone scan fits the transition contract"));
    rngCalls = 0;
    test.expectEq(Encounter::select(cache, party, chooseFirstSlot,
                                    Encounter::INERT_OFFSET, decision),
                  false, F("invalidated cache skips selection"));
    test.expectEq(rngCalls, static_cast<uint8_t>(0),
                  F("invalid cache does not roll"));

    // A wholly empty party refuses a valid cached zone without rolling.
    test.expectEq(Encounter::load(cache, chunk), true,
                  F("published zone reloads for the empty party"));
    for (uint8_t slot = 0; slot < PARTY_SIZE; ++slot) party[slot].level = 0;
    rngCalls = 0;
    test.expectEq(Encounter::select(cache, party, chooseFirstSlot,
                                    Encounter::INERT_OFFSET, decision),
                  false, F("empty party has no encounter"));
    test.expectEq(rngCalls, static_cast<uint8_t>(0), F("empty party has no roll"));
}
