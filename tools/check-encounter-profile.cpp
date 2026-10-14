#include "../src/engine/world/Encounter.hpp"
#include "../src/engine/world/Chunk.hpp"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

// Native authoring and packed-image validator for the wild encounter ABI in
// fxdata/data/encounters.txt and src/fxdata.h. It proves the content
// invariants that let Encounter::select drop its duplicate runtime scans:
// declared zone/table counts, active-zone chunk ids, table spans, lure mapping
// nibbles, the supported lure-type mask, species ids and inclusive level
// bounds. The packed mode additionally proves the published FX addresses point
// at the authored bytes in dist/fxdata-data.bin.
namespace {
// Mirrors Encounter.cpp's SPECIES_COUNT: the generated store holds 32 species.
constexpr uint8_t kSpeciesCount = 32;
// A zone's lure mask may only name the four supported lure types.
constexpr uint8_t kLureMaskBits =
    static_cast<uint8_t>((1u << Encounter::NIBBLE_COUNT) - 1);

bool parseArray(const std::string &source, const char *name,
                std::vector<uint8_t> &bytes, std::string &error) {
    const std::string declaration = std::string("uint8_t ") + name + "[]";
    const std::size_t found = source.find(declaration);
    if (found == std::string::npos) {
        error = std::string("missing byte array ") + name;
        return false;
    }
    const std::size_t afterName = found + declaration.size();
    const std::size_t equals = source.find('=', afterName);
    const std::size_t brace = source.find('{', afterName);
    if (equals == std::string::npos || brace == std::string::npos || brace < equals ||
        source.find_first_not_of(" \t\r\n", afterName) != equals ||
        source.find_first_not_of(" \t\r\n", equals + 1) != brace) {
        error = std::string(name) + " has no initializer";
        return false;
    }

    bytes.clear();
    std::size_t offset = brace + 1;
    while (offset < source.size()) {
        const unsigned char ch = static_cast<unsigned char>(source[offset]);
        if (std::isspace(ch) || ch == ',') {
            ++offset;
            continue;
        }
        if (source[offset] == '}') return true;
        if (source[offset] == '/' && offset + 1 < source.size() &&
            source[offset + 1] == '/') {
            offset = source.find('\n', offset + 2);
            if (offset == std::string::npos) break;
            continue;
        }
        if (source[offset] == '/' && offset + 1 < source.size() &&
            source[offset + 1] == '*') {
            const std::size_t end = source.find("*/", offset + 2);
            if (end == std::string::npos) break;
            offset = end + 2;
            continue;
        }
        if (!std::isdigit(ch)) {
            error = std::string(name) + " contains a non-decimal byte initializer";
            return false;
        }
        char *end = nullptr;
        const unsigned long value = std::strtoul(source.c_str() + offset, &end, 10);
        if (end == source.c_str() + offset || value > 255) {
            error = std::string(name) + " contains a value outside byte range";
            return false;
        }
        bytes.push_back(static_cast<uint8_t>(value));
        offset = static_cast<std::size_t>(end - source.c_str());
    }
    error = std::string(name) + " initializer is not closed";
    return false;
}

bool validateEncounterData(const std::vector<uint8_t> &zones,
                           const std::vector<uint8_t> &tables,
                           std::string &error) {
    if (zones.size() != Encounter::ZONE_COUNT * Encounter::ZONE_BYTES) {
        error = "zoneDefs must contain exactly the declared zone slots";
        return false;
    }
    if (tables.empty() || tables.size() % Encounter::TABLE_BYTES != 0) {
        error = "tables must contain whole nonempty encounter tables";
        return false;
    }
    const std::size_t tableCount = tables.size() / Encounter::TABLE_BYTES;
    if (tableCount != Encounter::TABLE_COUNT) {
        error = "tables count does not match Encounter::TABLE_COUNT";
        return false;
    }
    bool sentinelSeen = false;
    std::vector<uint16_t> chunkIds;
    for (uint8_t zone = 0; zone < Encounter::ZONE_COUNT; ++zone) {
        const std::size_t base = static_cast<std::size_t>(zone) * Encounter::ZONE_BYTES;
        const uint16_t chunkId = static_cast<uint16_t>(zones[base]) |
            (static_cast<uint16_t>(zones[base + 1]) << 8);
        if (chunkId == 0xFFFF) {
            sentinelSeen = true;
            for (uint8_t byte = 2; byte < Encounter::ZONE_BYTES; ++byte) {
                if (zones[base + byte] != 0) {
                    error = "sentinel zone fields after chunkId must be zero";
                    return false;
                }
            }
            continue;
        }
        if (sentinelSeen) {
            error = "active zones must precede the sentinel tail";
            return false;
        }
        if (!Chunk::validChunkId(chunkId)) {
            error = "active zone chunk ID exceeds the map chunk count";
            return false;
        }
        for (uint16_t previous : chunkIds) {
            if (previous == chunkId) {
                error = "active zone chunk IDs must be unique";
                return false;
            }
        }
        chunkIds.push_back(chunkId);

        const uint8_t tableBase = zones[base + 2];
        const uint8_t count = zones[base + 3];
        if (count == 0 || static_cast<std::size_t>(tableBase) + count > tableCount) {
            error = "active zone table span is outside tables";
            return false;
        }
        if ((zones[base + 4] & static_cast<uint8_t>(~kLureMaskBits)) != 0) {
            error = "active zone lure mask has unsupported bits";
            return false;
        }
        for (uint8_t lure = 0; lure < Encounter::NIBBLE_COUNT; ++lure) {
            if (zones[base + 5 + lure] >= count) {
                error = "active zone lure nibble is outside the table span";
                return false;
            }
        }
    }

    for (std::size_t table = 0; table < tableCount; ++table) {
        const std::size_t base = table * Encounter::TABLE_BYTES;
        for (uint8_t slot = 0; slot < Encounter::SLOT_COUNT; ++slot) {
            if (tables[base + slot] >= kSpeciesCount) {
                error = "table species ID is outside the generated roster";
                return false;
            }
        }
        const uint8_t floor = tables[base + Encounter::SLOT_COUNT];
        const uint8_t ceil = tables[base + Encounter::SLOT_COUNT + 1];
        if (floor == 0 || floor > ceil) {
            error = "table inclusive level bounds must be within 1..255";
            return false;
        }
    }
    return true;
}

bool validateEncounterSource(const std::string &source, std::string &error) {
    const std::size_t namespaceAt = source.find("namespace EncounterFXData");
    const std::size_t zoneAt = source.find("uint8_t zoneDefs[]");
    const std::size_t tableAt = source.find("uint8_t tables[]");
    if (namespaceAt == std::string::npos || zoneAt <= namespaceAt ||
        tableAt <= zoneAt ||
        source.find("uint8_t zoneDefs[]", zoneAt + 1) != std::string::npos ||
        source.find("uint8_t tables[]", tableAt + 1) != std::string::npos) {
        error = "expected one ordered EncounterFXData zoneDefs then tables declaration";
        return false;
    }
    std::vector<uint8_t> zones;
    std::vector<uint8_t> tables;
    return parseArray(source, "zoneDefs", zones, error) &&
           parseArray(source, "tables", tables, error) &&
           validateEncounterData(zones, tables, error);
}

bool readPublishedOffset(const std::string &header, const char *name,
                         std::size_t &offset, std::string &error) {
    const std::string marker = std::string(name) + " = 0x";
    const std::size_t found = header.find(marker);
    if (found == std::string::npos ||
        header.find(marker, found + marker.size()) != std::string::npos) {
        error = std::string("src/fxdata.h must publish one ") + name + " offset";
        return false;
    }
    const std::size_t first = found + marker.size();
    char *end = nullptr;
    const unsigned long value = std::strtoul(header.c_str() + first, &end, 16);
    if (end == header.c_str() + first || *end != ';') {
        error = std::string("invalid published offset for ") + name;
        return false;
    }
    offset = value;
    return true;
}

bool validatePackedEncounterData(const std::vector<uint8_t> &zones,
                                 const std::vector<uint8_t> &tables,
                                 const std::string &header,
                                 const std::vector<uint8_t> &packed,
                                 std::string &error) {
    std::size_t zoneOffset = 0;
    std::size_t tableOffset = 0;
    if (!readPublishedOffset(header, "zoneDefs", zoneOffset, error) ||
        !readPublishedOffset(header, "tables", tableOffset, error)) return false;
    if (tableOffset != zoneOffset + zones.size() ||
        zoneOffset + zones.size() > packed.size() ||
        tableOffset + tables.size() > packed.size()) {
        error = "published encounter ranges do not match the packed data layout";
        return false;
    }
    if (!std::equal(zones.begin(), zones.end(), packed.begin() + zoneOffset) ||
        !std::equal(tables.begin(), tables.end(), packed.begin() + tableOffset)) {
        error = "encounters.txt bytes differ from published FX image bytes";
        return false;
    }
    return true;
}

bool readTextFile(const char *path, std::string &source) {
    std::ifstream input(path);
    if (!input) {
        std::cerr << "encounter-profile: cannot open " << path << '\n';
        return false;
    }
    source.assign(std::istreambuf_iterator<char>(input),
                  std::istreambuf_iterator<char>());
    return !source.empty();
}

bool readBinaryFile(const char *path, std::vector<uint8_t> &bytes) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        std::cerr << "encounter-profile: cannot open " << path << '\n';
        return false;
    }
    bytes.assign(std::istreambuf_iterator<char>(input),
                 std::istreambuf_iterator<char>());
    return true;
}
} // namespace

#ifndef ENCOUNTER_PROFILE_TEST
int main(int argc, char **argv) {
    const bool sourceOnly = argc == 3 && std::string(argv[1]) == "--source";
    if (!sourceOnly && argc != 4) {
        std::cerr << "usage: check-encounter-profile --source <encounters.txt>\n"
                     "   or: check-encounter-profile <encounters.txt> "
                     "<src/fxdata.h> <fxdata-data.bin>\n";
        return 2;
    }
    std::string source;
    const char *sourcePath = sourceOnly ? argv[2] : argv[1];
    if (!readTextFile(sourcePath, source)) return 2;
    std::string error;
    if (!validateEncounterSource(source, error)) {
        std::cerr << "encounter-profile: " << error << '\n';
        return 1;
    }
    if (sourceOnly) {
        std::cout << "encounter-profile: authored zones and tables valid\n";
        return 0;
    }
    std::string header;
    std::vector<uint8_t> packed;
    if (!readTextFile(argv[2], header) ||
        !readBinaryFile(argv[3], packed)) return 2;
    std::vector<uint8_t> zones;
    std::vector<uint8_t> tables;
    if (!parseArray(source, "zoneDefs", zones, error) ||
        !parseArray(source, "tables", tables, error) ||
        !validatePackedEncounterData(zones, tables, header, packed, error)) {
        std::cerr << "encounter-profile: " << error << '\n';
        return 1;
    }
    std::cout << "encounter-profile: " << static_cast<unsigned>(Encounter::ZONE_COUNT)
              << " zones and " << static_cast<unsigned>(Encounter::TABLE_COUNT)
              << " declared tables valid\n";
    return 0;
}
#endif
