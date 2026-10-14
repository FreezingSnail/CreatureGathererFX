#define ENCOUNTER_PROFILE_TEST
#include "../check-encounter-profile.cpp"

#include <cstdlib>
#include <iostream>

namespace {
void require(bool condition, const char *message) {
    if (!condition) {
        std::cerr << "encounter-profile-test: " << message << '\n';
        std::exit(1);
    }
}

void setZone(std::vector<uint8_t> &zones, uint8_t index, uint16_t chunk,
             uint8_t tableBase = 0, uint8_t tableCount = 1) {
    const std::size_t base = static_cast<std::size_t>(index) * Encounter::ZONE_BYTES;
    zones[base] = static_cast<uint8_t>(chunk);
    zones[base + 1] = static_cast<uint8_t>(chunk >> 8);
    if (chunk == 0xFFFF) return;
    zones[base + 2] = tableBase;
    zones[base + 3] = tableCount;
    zones[base + 4] = 1;
    // Canonical lure-type mapping: nibble 1..4 selects tables 1..4 of the span.
    for (uint8_t lure = 0; lure < Encounter::NIBBLE_COUNT; ++lure) {
        zones[base + 5 + lure] = static_cast<uint8_t>(lure + 1);
    }
    zones[base + 9] = 0;
}

void expectRejected(std::vector<uint8_t> zones, std::vector<uint8_t> tables,
                    const char *diagnostic) {
    std::string error;
    if (validateEncounterData(zones, tables, error)) {
        std::cerr << "encounter-profile-test: accepted fixture expected to fail "
                  << "with diagnostic " << diagnostic << '\n';
        std::exit(1);
    }
    require(error.find(diagnostic) != std::string::npos,
            "invalid fixture has unexpected diagnostic");
}

std::vector<uint8_t> validZones() {
    std::vector<uint8_t> zones(Encounter::ZONE_COUNT * Encounter::ZONE_BYTES, 0);
    for (uint8_t zone = 0; zone < Encounter::ZONE_COUNT; ++zone) {
        setZone(zones, zone, 0xFFFF);
    }
    // The authored pocket floor carries the lure-zone table on both sides of
    // the chunk 1 / chunk 33 boundary.
    setZone(zones, 0, 0, 0, Encounter::TABLE_COUNT);
    setZone(zones, 1, 1, 0, Encounter::TABLE_COUNT);
    setZone(zones, 2, 33, 0, Encounter::TABLE_COUNT);
    return zones;
}

std::vector<uint8_t> validTables() {
    std::vector<uint8_t> tables(Encounter::TABLE_COUNT * Encounter::TABLE_BYTES, 0);
    for (uint8_t table = 0; table < Encounter::TABLE_COUNT; ++table) {
        const std::size_t base = static_cast<std::size_t>(table) * Encounter::TABLE_BYTES;
        for (uint8_t slot = 0; slot < Encounter::SLOT_COUNT; ++slot) {
            tables[base + slot] = static_cast<uint8_t>((table + slot) % kSpeciesCount);
        }
        tables[base + Encounter::SLOT_COUNT] = 1;
        tables[base + Encounter::SLOT_COUNT + 1] = 31;
    }
    return tables;
}
} // namespace

int main() {
    std::vector<uint8_t> zones = validZones();
    std::vector<uint8_t> tables = validTables();
    std::string error;
    require(validateEncounterData(zones, tables, error),
            "valid current zone and table set rejected");
    tables[0] = 0; // Species zero is a valid authored encounter.
    require(validateEncounterData(zones, tables, error), "species zero rejected");

    std::vector<uint8_t> noZones(Encounter::ZONE_COUNT * Encounter::ZONE_BYTES, 0);
    for (uint8_t zone = 0; zone < Encounter::ZONE_COUNT; ++zone) {
        setZone(noZones, zone, 0xFFFF);
    }
    require(validateEncounterData(noZones, validTables(), error),
            "all-sentinel no-zone source rejected");

    expectRejected(std::vector<uint8_t>(zones.size() - 1, 0), tables,
                   "zone slots");
    expectRejected(zones, {}, "whole nonempty");
    expectRejected(zones, std::vector<uint8_t>(Encounter::TABLE_BYTES - 1, 0),
                   "whole nonempty");
    std::vector<uint8_t> extraTable = validTables();
    const std::vector<uint8_t> secondTable = validTables();
    extraTable.insert(extraTable.end(), secondTable.begin(), secondTable.end());
    expectRejected(zones, extraTable, "TABLE_COUNT");

    std::vector<uint8_t> invalidZones = validZones();
    invalidZones[3] = 0;
    expectRejected(invalidZones, validTables(), "table span");
    invalidZones = validZones();
    invalidZones[2] = 1;
    expectRejected(invalidZones, validTables(), "table span");
    invalidZones = validZones();
    invalidZones[2] = 5;
    expectRejected(invalidZones, validTables(), "table span");
    invalidZones = validZones();
    setZone(invalidZones, 4, 7, 0, Encounter::TABLE_COUNT);
    expectRejected(invalidZones, validTables(), "sentinel tail");
    invalidZones = validZones();
    setZone(invalidZones, 1, 0);
    expectRejected(invalidZones, validTables(), "unique");
    invalidZones = validZones();
    invalidZones[3 * Encounter::ZONE_BYTES + 2] = 1;
    expectRejected(invalidZones, validTables(), "sentinel zone fields");
    invalidZones = validZones();
    setZone(invalidZones, 0, 2048);
    expectRejected(invalidZones, validTables(), "chunk ID");

    // Lure mapping and mask: every nibble must fall inside the zone's span and
    // the mask may only name the four supported lure types.
    invalidZones = validZones();
    invalidZones[5] = Encounter::TABLE_COUNT;
    expectRejected(invalidZones, validTables(), "lure nibble");
    invalidZones = validZones();
    invalidZones[3] = 1;
    invalidZones[5] = 1;
    expectRejected(invalidZones, validTables(), "lure nibble");
    invalidZones = validZones();
    invalidZones[4] = 0x10;
    expectRejected(invalidZones, validTables(), "lure mask");
    invalidZones = validZones();
    for (uint8_t lure = 0; lure < Encounter::NIBBLE_COUNT; ++lure) {
        invalidZones[5 + lure] = 0; // Nibble zero keeps the base table.
    }
    require(validateEncounterData(invalidZones, validTables(), error),
            "base-table nibble mapping rejected");

    std::vector<uint8_t> invalidTables = validTables();
    invalidTables[0] = 32;
    expectRejected(validZones(), invalidTables, "species ID");
    invalidTables = validTables();
    invalidTables[Encounter::SLOT_COUNT] = 0;
    expectRejected(validZones(), invalidTables, "level bounds");
    invalidTables = validTables();
    invalidTables[Encounter::SLOT_COUNT] = 31;
    invalidTables[Encounter::SLOT_COUNT + 1] = 30;
    expectRejected(validZones(), invalidTables, "level bounds");
    invalidTables = validTables();
    const std::size_t lureBase =
        static_cast<std::size_t>(Encounter::TABLE_COUNT - 1) * Encounter::TABLE_BYTES;
    invalidTables[lureBase + Encounter::SLOT_COUNT + 1] = 255;
    require(validateEncounterData(validZones(), invalidTables, error),
            "maximum byte-sized ceiling rejected");

    const auto arraySource = [](const char *name, const std::vector<uint8_t> &values) {
        std::string source = std::string("uint8_t ") + name + "[] = {";
        for (uint8_t value : values) {
            source += " " + std::to_string(value) + ",";
        }
        source += " };\n";
        return source;
    };
    const std::string source = "namespace EncounterFXData {\n" +
        arraySource("zoneDefs", validZones()) +
        arraySource("tables", validTables()) + "}\n";
    require(validateEncounterSource(source, error), "valid source arrays rejected");
    require(!validateEncounterSource("namespace EncounterFXData {}", error),
            "missing source arrays accepted");

    const std::vector<uint8_t> packedZones = validZones();
    const std::vector<uint8_t> packedTables = validTables();
    std::vector<uint8_t> packed(Encounter::ZONE_COUNT * Encounter::ZONE_BYTES +
                                Encounter::TABLE_COUNT * Encounter::TABLE_BYTES, 0xA5);
    std::copy(packedZones.begin(), packedZones.end(), packed.begin());
    std::copy(packedTables.begin(), packedTables.end(),
              packed.begin() + Encounter::ZONE_COUNT * Encounter::ZONE_BYTES);
    const std::string header =
        "namespace EncounterFXData {\n"
        "constexpr uint24_t zoneDefs = 0x000000;\n"
        "constexpr uint24_t tables = 0x000050;\n}\n";
    require(validatePackedEncounterData(packedZones, packedTables, header,
                                        packed, error),
            "matching packed image rejected");
    packed[Encounter::ZONE_COUNT * Encounter::ZONE_BYTES] ^= 1;
    require(!validatePackedEncounterData(packedZones, packedTables, header,
                                         packed, error),
            "stale packed table byte accepted");
    const std::string badHeader =
        "namespace EncounterFXData {\n"
        "constexpr uint24_t zoneDefs = 0x000000;\n"
        "constexpr uint24_t tables = 0x000051;\n}\n";
    require(!validatePackedEncounterData(packedZones, packedTables, badHeader,
                                         packed, error),
            "incorrect published offset accepted");

    std::cout << "encounter-profile-test: authoring tables, bounds, lures and sentinels passed\n";
}
