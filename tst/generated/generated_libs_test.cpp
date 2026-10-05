// Integration test for the cgfx-tools generated FX libraries.
//
// Contract under test: for every entry in fxlayout.toml, the bytes of its
// source appear in the packed image at exactly the address src/fxdata.h
// publishes, in the encoding the firmware's read path assumes:
//
//   * carray entries  - uint8_t initializers, literal byte order
//   * symbol entries  - typed tables (uint8_t / uint24_t / uint32_t) whose
//                       symbolic items are packed big-endian, which is the
//                       order FX::readIndexedUInt24() reassembles
//   * raw entries     - byte-for-byte copies of the generated .bin
//
// The decoders here are written from the binary format, independent of the
// packer's own encoders, so a codegen or packer regression shows up as a
// mismatch instead of two implementations agreeing on the same mistake.
//
// Usage: generated_libs_test <repo-root> [layout] [header] [image]

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <optional>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

#include "../../src/engine/world/Chunk.hpp"
#include "../../src/engine/world/TileProps.hpp"

namespace {

struct Failure {
    std::string label;
    std::string detail;
};

class Report {
  public:
    void pass() { ++passes_; }

    void fail(const std::string &label, const std::string &detail) {
        failures_.push_back({label, detail});
    }

    int finish(const char *suite) const {
        for (const Failure &failure : failures_) {
            std::cout << "FAIL " << failure.label << "\n     " << failure.detail << "\n";
        }
        std::cout << suite << ": " << passes_ << " passed, " << failures_.size() << " failed\n";
        if (!failures_.empty()) {
            std::cout << "remedy: run make gen; a persistent mismatch is a codegen or packer defect\n";
            return 1;
        }
        std::cout << suite << ": PASS\n";
        return 0;
    }

  private:
    std::size_t passes_ = 0;
    std::vector<Failure> failures_;
};

std::optional<std::string> readFile(const std::string &path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) return std::nullopt;
    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

std::map<std::string, uint8_t> canonicalIds(const std::string &path, bool moves) {
    const auto contents = readFile(path);
    if (!contents) return {};
    std::map<std::string, uint8_t> result;
    int depth = 0;
    bool inString = false, escaped = false;
    std::size_t start = 0;
    for (std::size_t i = 0; i < contents->size(); ++i) {
        const char c = (*contents)[i];
        if (inString) {
            if (escaped) escaped = false;
            else if (c == '\\') escaped = true;
            else if (c == '"') inString = false;
            continue;
        }
        if (c == '"') { inString = true; continue; }
        if (c == '{') {
            if (depth++ == 0) start = i;
        } else if (c == '}' && --depth == 0) {
            const std::string object = contents->substr(start, i - start + 1);
            std::smatch name, id;
            const std::regex namePattern("\\\"name\\\"\\s*:\\s*\\\"([^\\\"]+)\\\"");
            const std::regex idPattern("\\\"id\\\"\\s*:\\s*([0-9]+)");
            if (std::regex_search(object, name, namePattern) &&
                std::regex_search(object, id, idPattern)) {
                result.emplace(name[1].str(), static_cast<uint8_t>(std::stoul(id[1].str())));
            }
        }
    }
    (void)moves;
    return result;
}

std::vector<std::vector<std::string>> csvRows(const std::string &path) {
    const auto contents = readFile(path);
    std::vector<std::vector<std::string>> rows;
    if (!contents) return rows;
    std::istringstream lines(*contents);
    std::string line;
    while (std::getline(lines, line)) {
        std::vector<std::string> row;
        std::istringstream columns(line);
        std::string cell;
        while (std::getline(columns, cell, ',')) row.push_back(cell);
        rows.push_back(std::move(row));
    }
    return rows;
}

std::optional<uint8_t> canonicalLookup(const std::map<std::string, uint8_t> &ids,
                                       const std::string &name, bool species) {
    auto exact = ids.find(name);
    if (exact != ids.end()) return exact->second;
    if (!species) return std::nullopt;
    for (const auto &[canonical, id] : ids) {
        if (canonical.size() != name.size()) continue;
        bool same = true;
        for (std::size_t i = 0; i < name.size(); ++i)
            if (std::tolower(static_cast<unsigned char>(canonical[i])) !=
                std::tolower(static_cast<unsigned char>(name[i]))) same = false;
        if (same) return id;
    }
    return std::nullopt;
}

struct AddressTable;
std::optional<uint32_t> resolve(const AddressTable &table, const std::string &name,
                                const std::string &space);

template <typename DecoderT>
void checkArenaCatalog(Report &report, const std::string &root, const AddressTable &addresses,
                       const std::string &image) {
    const auto descriptor = readFile(root + "/fxdata/generated/arena_demo.txt");
    const auto idsHeader = readFile(root + "/fxdata/generated/arena_demo_ids.hpp");
    if (!descriptor || !idsHeader) {
        report.fail("arena catalog", "missing generated descriptor or symbolic IDs; run make gen");
        return;
    }
    DecoderT decoder(*descriptor);
    const auto creatures = canonicalIds(root + "/data/json/creatures.json", false);
    const auto moves = canonicalIds(root + "/data/json/moves.json", true);
    if (creatures.empty() || moves.empty()) {
        report.fail("arena catalog", "canonical creature or move source is missing/empty");
        return;
    }
    auto idConstant = [&](const std::string &name) -> std::optional<uint8_t> {
        std::smatch match;
        const std::regex pattern("constexpr uint8_t " + name + " = ([0-9]+);");
        if (!std::regex_search(*idsHeader, match, pattern)) return std::nullopt;
        return static_cast<uint8_t>(std::stoul(match[1].str()));
    };
    auto getArray = [&](const std::string &name) -> std::optional<std::vector<uint8_t>> {
        return decoder.carrayBytes(name);
    };
    auto checkImageRecord = [&](const std::string &symbol, const std::vector<uint8_t> &bytes) {
        const auto address = resolve(addresses, symbol, "ArenaDemoData");
        if (!address || *address + bytes.size() > image.size()) {
            report.fail("arena packed " + symbol, "missing address or record runs past payload");
            return;
        }
        if (!std::equal(bytes.begin(), bytes.end(), image.begin() + *address,
                        [](uint8_t expected, char actual) {
                            return expected == static_cast<uint8_t>(actual);
                        })) {
            report.fail("arena packed " + symbol, "descriptor bytes differ from packed image");
            return;
        }
        report.pass();
    };
    const std::array<std::string, 3> playerKeys{"blitz", "bulwark", "utility"};
    const std::array<std::array<std::array<const char *, 5>, 3>, 3> players{{
        {{{"ScrambleSnail","breeze","bellow","torrent","seedfall"}, {"ScatterCrab","dirtburst","slipfall","dirtfall","none"}, {"bigsquid","splash","squirt","soak","none"}}},
        {{{"rock","dirtburst","wisper","Ironbody","slipfall"}, {"ShatterCrab","slipfall","bolt","plasma","dirtburst"}, {"item2","wisper","burst","splash","slam"}}},
        {{{"bell","wisper","Sharpen","none","none"}, {"hedge","root","rejuvinate","pollen","none"}, {"cloud","blow","zap","sweep","torrent"}}}
    }};
    std::vector<uint8_t> playerBytes;
    for (std::size_t t = 0; t < playerKeys.size(); ++t) {
        const auto symbolic = idConstant("player_" + playerKeys[t]);
        if (!symbolic || *symbolic != t) report.fail("arena player IDs", "symbolic player IDs are not contiguous");
        for (const auto &member : players[t]) {
            const auto species = canonicalLookup(creatures, member[0], true);
            if (!species) { report.fail("arena roster", "unknown canonical species " + std::string(member[0])); continue; }
            playerBytes.push_back(*species);
            playerBytes.push_back(31);
            for (std::size_t m = 1; m < member.size(); ++m) {
                const auto move = std::string(member[m]) == "none"
                                      ? std::optional<uint8_t>(32)
                                      : canonicalLookup(moves, member[m], false);
                if (!move) report.fail("arena roster", "unknown canonical move " + std::string(member[m]));
                playerBytes.push_back(move.value_or(0));
            }
        }
    }
    if (playerBytes.size() != 3 * 18) report.fail("arena player shape", "expected exactly three 18-byte player records");
    auto packedPlayers = getArray("playerMembers");
    if (!packedPlayers || *packedPlayers != playerBytes) report.fail("arena player order", "authored species/moves/level order differs from six-byte records");
    else report.pass();
    checkImageRecord("playerMembers", playerBytes);

    const std::array<std::string, 5> opponentKeys{"starter", "speed", "fortress", "tricks", "champion"};
    const std::array<std::array<const char *, 3>, 5> opponentSpecies{{
        {{"SkitterCrab","squid","SquibbleSnail"}}, {{"skimray","waggleworm","flitfly"}},
        {{"rock","ShatterCrab","BiggestSquid"}}, {{"bell","hedge","cloud"}},
        {{"ardu","dragon","skull"}}
    }};
    const auto csv = csvRows(root + "/data/opponents.csv");
    const auto opponentsDescriptor = readFile(root + "/fxdata/generated/opponents.txt");
    std::optional<DecoderT> opponentDecoder;
    if (opponentsDescriptor) opponentDecoder.emplace(*opponentsDescriptor);
    const auto opts = opponentDecoder ? opponentDecoder->symbolBytes("opts", "", addresses) : std::nullopt;
    const auto packedTrainerIds = getArray("opponentTrainerIds");
    std::vector<uint8_t> trainers, speciesRows;
    for (std::size_t t = 0; t < opponentKeys.size(); ++t) {
        const auto teamId = idConstant("opponent_" + opponentKeys[t]);
        const auto trainerId = idConstant("trainer_" + opponentKeys[t]);
        if (!teamId || *teamId != t || !trainerId) report.fail("arena opponent IDs", "missing/noncontiguous generated symbolic opponent IDs");
        const uint8_t trainer = trainerId.value_or(255);
        trainers.push_back(trainer);
        std::array<uint8_t, 3> speciesIds{};
        for (std::size_t s = 0; s < 3; ++s) {
            const auto id = canonicalLookup(creatures, opponentSpecies[t][s], true);
            if (!id) report.fail("arena opponent species", "unknown canonical species " + std::string(opponentSpecies[t][s]));
            speciesIds[s] = id.value_or(0);
            speciesRows.push_back(speciesIds[s]);
        }
        auto sourceRow = csv.end();
        for (auto row = csv.begin() + std::min<std::size_t>(1, csv.size()); row != csv.end(); ++row) {
            if (row->size() < 19) continue;
            bool match = true;
            for (std::size_t s = 0; s < 3; ++s) {
                const auto rowSpecies = canonicalLookup(creatures, (*row)[s], true);
                if (!rowSpecies || *rowSpecies != speciesIds[s]) match = false;
            }
            if (match) {
                if (sourceRow != csv.end()) report.fail("arena trainer row", "species triple is not unique in opponents.csv");
                sourceRow = row;
            }
        }
        if (sourceRow == csv.end()) {
            report.fail("arena trainer row", "catalog species triple has no authored opponents.csv row");
            continue;
        }
        std::vector<uint8_t> expected;
        for (std::size_t s = 0; s < 3; ++s) {
            const auto id = canonicalLookup(creatures, (*sourceRow)[s], true);
            if (!id || *id != speciesIds[s]) report.fail("arena trainer species", "CSV trainer species differs from catalog order");
            expected.push_back(speciesIds[s]);
            if ((*sourceRow)[s + 3] != "31") report.fail("arena trainer level", "arena opponent is not level 31");
            expected.push_back(31);
            for (std::size_t m = 0; m < 4; ++m) {
                const std::string &moveName = (*sourceRow)[7 + s * 4 + m];
                const auto move = moveName == "none" ? std::optional<uint8_t>(255)
                                                       : canonicalLookup(moves, moveName, false);
                if (!move) report.fail("arena trainer move", "unknown canonical move " + moveName);
                expected.push_back(move.value_or(255));
            }
        }
        const auto seed = opponentDecoder ? opponentDecoder->carrayBytes(trainer == 0 ? "opponent_seeds" : "opponent_seed_" + std::to_string(trainer)) : std::nullopt;
        if (!seed || *seed != expected) report.fail("arena trainer record", "resolved opts row differs from canonical CSV records");
        else report.pass();
        if (opts && (static_cast<std::size_t>(trainer) + 1) * 3 <= opts->size()) {
            const std::size_t at = static_cast<std::size_t>(trainer) * 3;
            const uint32_t packedAddress = (uint32_t((*opts)[at]) << 16) | (uint32_t((*opts)[at + 1]) << 8) | (*opts)[at + 2];
            const std::string target = trainer == 0 ? "opponent_seeds" : "opponent_seed_" + std::to_string(trainer);
            const auto expectedAddress = resolve(addresses, target, "");
            if (!expectedAddress || packedAddress != *expectedAddress) report.fail("arena opts resolution", "trainer ID does not resolve the existing opts address table");
            else report.pass();
        } else report.fail("arena opts resolution", "trainer index exceeds existing opts table");
    }
    if (!packedTrainerIds || *packedTrainerIds != trainers) report.fail("arena trainer IDs", "catalog trainer IDs differ from symbolic IDs");
    else report.pass();
    if (speciesRows.size() != 15) report.fail("arena species shape", "expected five three-species opponent rows");
    auto packedSpecies = getArray("opponentSpecies");
    if (!packedSpecies || *packedSpecies != speciesRows) report.fail("arena species rows", "opponentSpecies does not match canonical roster rows");
    else report.pass();
    checkImageRecord("opponentTrainerIds", trainers);
    checkImageRecord("opponentSpecies", speciesRows);

    const std::array<std::string, 3> playerLabels{"Blitz", "Bulwark", "Utility"};
    const std::array<std::string, 5> opponentLabels{"Starter", "Speed", "Fortress", "Tricks", "Champion"};
    auto checkLabels = [&](const std::string &table, const std::vector<std::string> &labels,
                           const std::string &widthTable) {
        const auto addressTable = decoder.symbolBytes(table, "ArenaDemoData", addresses);
        const auto widths = getArray(widthTable);
        if (!addressTable || addressTable->size() != labels.size() * 3 || !widths || widths->size() != labels.size()) {
            report.fail("arena labels " + table, "label address/width table cardinality mismatch");
            return;
        }
        for (std::size_t i = 0; i < labels.size(); ++i) {
            const std::string key = (table == "playerLabels" ? "player_" : "opponent_") +
                                    (table == "playerLabels" ? playerKeys[i] : opponentKeys[i]);
            const auto labelAddress = resolve(addresses, key, "ArenaDemoData");
            const uint32_t encoded = (uint32_t((*addressTable)[i * 3]) << 16) |
                                     (uint32_t((*addressTable)[i * 3 + 1]) << 8) |
                                     (*addressTable)[i * 3 + 2];
            const std::string labelSymbol = (table == "playerLabels" ? "player_" : "opponent_") +
                                            (table == "playerLabels" ? playerKeys[i] : opponentKeys[i]);
            const auto bitmap = getArray(labelSymbol);
            const uint8_t expectedWidth = static_cast<uint8_t>(labels[i].size() * 5);
            if (!labelAddress || encoded != *labelAddress || (*widths)[i] != expectedWidth ||
                !bitmap || bitmap->size() != static_cast<std::size_t>(expectedWidth) * 2) {
                report.fail("arena label " + labels[i], "symbol address, 5-pixel glyph width, or two headerless 8px frames mismatch");
                continue;
            }
            if (*labelAddress + bitmap->size() > image.size() ||
                !std::equal(bitmap->begin(), bitmap->end(), image.begin() + *labelAddress,
                            [](uint8_t expected, char actual) {
                                return expected == static_cast<uint8_t>(actual);
                            })) {
                std::size_t mismatch = 0;
                while (mismatch < bitmap->size() && *labelAddress + mismatch < image.size() &&
                       (*bitmap)[mismatch] == static_cast<uint8_t>(image[*labelAddress + mismatch])) ++mismatch;
                report.fail("arena label " + labels[i], "bitmap mismatch at " + std::to_string(mismatch) +
                    " address=" + std::to_string(*labelAddress) + " bytes=" + std::to_string(bitmap->size()) +
                    " image=" + std::to_string(image.size()));
                continue;
            }
            report.pass();
        }
        checkImageRecord(table, *addressTable);
        checkImageRecord(widthTable, *widths);
    };
    checkLabels("playerLabels", {playerLabels.begin(), playerLabels.end()}, "playerLabelWidths");
    checkLabels("opponentLabels", {opponentLabels.begin(), opponentLabels.end()}, "opponentLabelWidths");
}

// ---------------------------------------------------------------------------
// src/fxdata.h address table
// ---------------------------------------------------------------------------

// Keys match what the packer resolves against: namespaced fields are joined
// with "::", top-level fields stay bare.
struct AddressTable {
    std::map<std::string, uint32_t> qualified;
    // The packer (inherited from fxdata-build.py) resolves an unqualified
    // reference to the *first* declaration with that name even when that
    // declaration is namespaced, so `attackText` in a table means
    // MenuFXData::attackText rather than the later top-level attackText.
    std::map<std::string, uint32_t> firstByName;
};

AddressTable parseAddresses(const std::string &header) {
    AddressTable table;
    std::vector<std::string> scope;
    std::istringstream lines(header);
    std::string line;
    while (std::getline(lines, line)) {
        std::istringstream tokens(line);
        std::string first;
        tokens >> first;
        if (first == "namespace") {
            std::string name;
            tokens >> name;
            if (!name.empty()) scope.push_back(name);
            continue;
        }
        if (first == "}" && !scope.empty()) {
            scope.pop_back();
            continue;
        }
        if (first != "constexpr") continue;

        std::string type, name, equals, value;
        tokens >> type >> name >> equals >> value;
        if (type != "uint24_t" || equals != "=") continue;
        if (!value.empty() && value.back() == ';') value.pop_back();
        // FX_* are section sizes and save_* are save-relative offsets.
        if (name.rfind("FX_", 0) == 0 || name.rfind("save_", 0) == 0) continue;

        std::string key;
        for (const std::string &component : scope) key += component + "::";
        key += name;
        const auto address = static_cast<uint32_t>(std::stoul(value, nullptr, 0));
        table.qualified[key] = address;
        table.firstByName.emplace(name, address);
    }
    return table;
}

// Mirrors the packer's lookup order: qualified, namespace-local, first
// declaration of that name, then a unique unqualified suffix.
std::optional<uint32_t> resolve(const AddressTable &table, const std::string &name,
                                const std::string &space) {
    if (name.find("::") != std::string::npos) {
        auto found = table.qualified.find(name);
        if (found != table.qualified.end()) return found->second;
        return std::nullopt;
    }
    if (!space.empty()) {
        auto found = table.qualified.find(space + "::" + name);
        if (found != table.qualified.end()) return found->second;
    }
    auto found = table.firstByName.find(name);
    if (found != table.firstByName.end()) return found->second;

    std::optional<uint32_t> unique;
    for (const auto &[key, value] : table.qualified) {
        const std::size_t separator = key.rfind("::");
        const std::string suffix = separator == std::string::npos ? key : key.substr(separator + 2);
        if (suffix != name) continue;
        if (unique) return std::nullopt; // ambiguous
        unique = value;
    }
    return unique;
}

// ---------------------------------------------------------------------------
// fxlayout.toml entries
// ---------------------------------------------------------------------------

struct Entry {
    std::string name;
    std::string space;
    std::string kind; // raw | carray | symbol | image
    std::string path;
    std::string symbol;

    std::string key() const { return space.empty() ? name : space + "::" + name; }
};

std::vector<std::string> quotedValues(const std::string &line) {
    std::vector<std::string> values;
    std::size_t position = 0;
    while ((position = line.find('"', position)) != std::string::npos) {
        const std::size_t end = line.find('"', position + 1);
        if (end == std::string::npos) break;
        values.push_back(line.substr(position + 1, end - position - 1));
        position = end + 1;
    }
    return values;
}

std::vector<Entry> parseLayout(const std::string &layout) {
    std::vector<Entry> entries;
    bool inEntry = false;
    Entry current;
    auto flush = [&]() {
        if (!current.name.empty()) entries.push_back(current);
        current = Entry{};
    };

    std::istringstream lines(layout);
    std::string line;
    while (std::getline(lines, line)) {
        const std::size_t start = line.find_first_not_of(" \t");
        const std::string trimmed = start == std::string::npos ? "" : line.substr(start);
        if (trimmed.rfind("[[entry]]", 0) == 0) {
            flush();
            inEntry = true;
            continue;
        }
        if (!trimmed.empty() && trimmed[0] == '[') {
            flush();
            inEntry = false;
            continue;
        }
        if (!inEntry) continue;

        const std::vector<std::string> values = quotedValues(trimmed);
        if (trimmed.rfind("name", 0) == 0 && trimmed.rfind("namespace", 0) != 0) {
            if (!values.empty()) current.name = values[0];
            continue;
        }
        if (trimmed.rfind("namespace", 0) == 0) {
            if (!values.empty()) current.space = values[0];
            continue;
        }
        if (trimmed.rfind("source", 0) != 0) continue;

        if (trimmed.find("carray") != std::string::npos) {
            current.kind = "carray";
        } else if (trimmed.find("symbol") != std::string::npos) {
            current.kind = "symbol";
        } else if (trimmed.find("image") != std::string::npos) {
            current.kind = "image";
        } else if (trimmed.find("raw") != std::string::npos) {
            current.kind = "raw";
        }
        if (!values.empty()) current.path = values[0];
        if (values.size() > 1) current.symbol = values[1];
    }
    flush();
    return entries;
}

// ---------------------------------------------------------------------------
// Source decoders
// ---------------------------------------------------------------------------

class Decoder {
  public:
    explicit Decoder(const std::string &source) : source_(source) {}

    const std::string &error() const { return error_; }

    // All uint8_t initializers, or just the named one.
    std::optional<std::vector<uint8_t>> carrayBytes(const std::string &symbol) {
        bytes_.clear();
        position_ = 0;
        found_ = false;
        while (position_ < source_.size()) {
            skipBlanks();
            if (position_ >= source_.size()) break;
            if (current() == '"') {
                if (!skipString()) return std::nullopt;
                continue;
            }
            if (isIdentifierStart(current())) {
                const std::string identifier = readIdentifier();
                if (identifier == "uint8_t") {
                    if (!carrayDeclaration(symbol)) return std::nullopt;
                    if (!symbol.empty() && found_) return bytes_;
                }
                continue;
            }
            ++position_;
        }
        if (!symbol.empty() && !found_) {
            error_ = "symbol " + symbol + " not found";
            return std::nullopt;
        }
        return bytes_;
    }

    // One typed declaration, packed big-endian at its declaration width.
    std::optional<std::vector<uint8_t>> symbolBytes(const std::string &symbol,
                                                    const std::string &space,
                                                    const AddressTable &addresses) {
        bytes_.clear();
        position_ = 0;
        found_ = false;
        while (position_ < source_.size()) {
            skipBlanks();
            if (position_ >= source_.size()) break;
            if (current() == '"') {
                if (!skipString()) return std::nullopt;
                continue;
            }
            if (!isIdentifierStart(current())) {
                ++position_;
                continue;
            }
            const std::size_t identifierStart = position_;
            const std::string identifier = readQualifiedIdentifier();

            // An untyped assignment is packed as a uint24_t table.
            if (identifier == symbol) {
                skipBlanks();
                if (position_ < source_.size() && current() == '=') {
                    ++position_;
                    found_ = true;
                    if (!typedInitializer(3, space, addresses)) return std::nullopt;
                    return bytes_;
                }
                continue;
            }

            const std::size_t width = declarationWidth(identifier);
            if (width == 0) continue;

            skipBlanks();
            while (position_ < source_.size() && (current() == '[' || current() == ']')) {
                ++position_;
                skipBlanks();
            }
            if (position_ >= source_.size() || !isIdentifierStart(current())) {
                position_ = identifierStart + identifier.size();
                continue;
            }
            const std::string name = readIdentifier();
            if (name != symbol) continue;

            while (position_ < source_.size() && current() != '=') ++position_;
            if (position_ >= source_.size()) {
                error_ = "symbol " + symbol + " has no initializer";
                return std::nullopt;
            }
            ++position_;
            found_ = true;
            if (!typedInitializer(width, space, addresses)) return std::nullopt;
            return bytes_;
        }
        error_ = "symbol " + symbol + " not found";
        return std::nullopt;
    }

  private:
    static bool isIdentifierStart(char c) {
        return c == '_' || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
    }
    static bool isIdentifier(char c) { return isIdentifierStart(c) || (c >= '0' && c <= '9'); }
    static bool isDigit(char c) { return c >= '0' && c <= '9'; }
    static bool isHex(char c) {
        return isDigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
    }

    char current() const { return source_[position_]; }
    bool starts(const char *text) const { return source_.compare(position_, strlenOf(text), text) == 0; }
    static std::size_t strlenOf(const char *text) {
        std::size_t length = 0;
        while (text[length] != '\0') ++length;
        return length;
    }

    void skipBlanks() {
        while (position_ < source_.size()) {
            while (position_ < source_.size() &&
                   (current() == ' ' || current() == '\t' || current() == '\n' ||
                    current() == '\r' || current() == '\f' || current() == '\v')) {
                ++position_;
            }
            if (starts("//")) {
                position_ += 2;
                while (position_ < source_.size() && current() != '\n') ++position_;
                continue;
            }
            if (starts("/*")) {
                position_ += 2;
                while (position_ < source_.size() && !starts("*/")) ++position_;
                position_ = std::min(position_ + 2, source_.size());
                continue;
            }
            return;
        }
    }

    // Skip spaces, tabs, and comments only. Returns true when the line ends
    // first, leaving the position on the newline.
    bool skipBlanksToLineEnd() {
        while (position_ < source_.size()) {
            const char byte = current();
            if (byte == '\n') return true;
            if (byte == ' ' || byte == '\t' || byte == '\r' || byte == '\f' || byte == '\v') {
                ++position_;
                continue;
            }
            if (starts("//")) {
                position_ += 2;
                while (position_ < source_.size() && current() != '\n') ++position_;
                return true;
            }
            if (starts("/*")) {
                position_ += 2;
                while (position_ < source_.size() && !starts("*/")) ++position_;
                position_ = std::min(position_ + 2, source_.size());
                continue;
            }
            return false;
        }
        return true;
    }

    std::string readIdentifier() {        const std::size_t start = position_;
        ++position_;
        while (position_ < source_.size() && isIdentifier(current())) ++position_;
        return source_.substr(start, position_ - start);
    }

    std::string readQualifiedIdentifier() {
        std::string name = readIdentifier();
        while (position_ + 1 < source_.size() && current() == ':' && source_[position_ + 1] == ':') {
            position_ += 2;
            if (position_ >= source_.size() || !isIdentifierStart(current())) return name;
            name += "::" + readIdentifier();
        }
        return name;
    }

    std::optional<uint32_t> readNumber() {
        const bool hexadecimal = starts("0x") || starts("0X");
        const bool binary = starts("0b") || starts("0B");
        const std::size_t start = position_;
        if (hexadecimal || binary) position_ += 2;
        const std::size_t digits = position_;
        while (position_ < source_.size() &&
               (hexadecimal ? isHex(current()) : isDigit(current()))) {
            ++position_;
        }
        if (position_ == digits) {
            error_ = "invalid numeric literal";
            return std::nullopt;
        }
        const std::string literal = source_.substr(digits, position_ - digits);
        const int base = hexadecimal ? 16 : (binary ? 2 : 10);
        (void)start;
        return static_cast<uint32_t>(std::stoul(literal, nullptr, base));
    }

    bool skipString() {
        ++position_;
        while (position_ < source_.size()) {
            const char byte = source_[position_++];
            if (byte == '"') return true;
            if (byte == '\\') ++position_;
        }
        error_ = "unterminated string literal";
        return false;
    }

    bool decodeString() {
        ++position_;
        while (position_ < source_.size()) {
            const char byte = source_[position_++];
            if (byte == '"') return true;
            if (byte != '\\') {
                bytes_.push_back(static_cast<uint8_t>(byte));
                continue;
            }
            if (position_ >= source_.size()) break;
            const char escape = source_[position_++];
            switch (escape) {
                case '0': bytes_.push_back(0); break;
                case 'n': bytes_.push_back('\n'); break;
                case 'r': bytes_.push_back('\r'); break;
                case 't': bytes_.push_back('\t'); break;
                case '\\': bytes_.push_back('\\'); break;
                case '"': bytes_.push_back('"'); break;
                case 'x': {
                    if (position_ + 1 >= source_.size() || !isHex(source_[position_]) ||
                        !isHex(source_[position_ + 1])) {
                        error_ = "invalid \\x escape";
                        return false;
                    }
                    const std::string digits = source_.substr(position_, 2);
                    position_ += 2;
                    bytes_.push_back(static_cast<uint8_t>(std::stoul(digits, nullptr, 16)));
                    break;
                }
                default:
                    error_ = std::string("unknown string escape \\") + escape;
                    return false;
            }
        }
        error_ = "unterminated string literal";
        return false;
    }

    bool carrayDeclaration(const std::string &symbol) {
        skipBlanks();
        if (position_ >= source_.size() || !isIdentifierStart(current())) {
            error_ = "uint8_t declaration must name an identifier";
            return false;
        }
        const std::string name = readIdentifier();
        while (true) {
            skipBlanks();
            if (position_ >= source_.size() || current() == ';') return true;
            if (current() == '=') {
                ++position_;
                skipBlanks();
                // A non-matching declaration is skipped without inspecting its
                // initializer, so unrelated forms in the same file cannot fail
                // the lookup (e.g. `uint8_t m1submapcount = 1`).
                if (!symbol.empty() && name != symbol) return true;
                found_ = true;
                if (position_ >= source_.size()) {
                    error_ = "unterminated uint8_t declaration";
                    return false;
                }
                if (current() == '{') return bracedBytes(true);
                if (current() == '"') return stringInitializer(true);
                error_ = "uint8_t initializer must start with '{' or a string literal";
                return false;
            }
            if (current() == '"') {
                if (!skipString()) return false;
            } else {
                ++position_;
            }
        }
    }

    bool bracedBytes(bool keep) {
        std::size_t depth = 0;
        while (true) {
            skipBlanks();
            if (position_ >= source_.size()) {
                error_ = "unterminated uint8_t initializer";
                return false;
            }
            const char byte = current();
            if (byte == '{') {
                ++depth;
                ++position_;
                continue;
            }
            if (byte == '}') {
                --depth;
                ++position_;
                if (depth == 0) return true;
                continue;
            }
            if (byte == '"') {
                if (!(keep ? decodeString() : skipString())) return false;
                continue;
            }
            if (isDigit(byte)) {
                const std::optional<uint32_t> value = readNumber();
                if (!value) return false;
                if (*value > 0xff) {
                    error_ = "value out of byte range in uint8_t initializer";
                    return false;
                }
                if (keep) bytes_.push_back(static_cast<uint8_t>(*value));
                continue;
            }
            if (byte == ',') {
                ++position_;
                continue;
            }
            if (isIdentifierStart(byte)) {
                error_ = "unsupported symbolic value " + readIdentifier() + " in uint8_t initializer";
                return false;
            }
            error_ = std::string("invalid byte '") + byte + "' in uint8_t initializer";
            return false;
        }
    }

    bool stringInitializer(bool keep) {
        if (!(keep ? decodeString() : skipString())) return false;
        while (true) {
            skipBlanks();
            if (position_ >= source_.size() || current() == ';') return true;
            if (current() != '"') return true;
            if (!(keep ? decodeString() : skipString())) return false;
        }
    }

    bool typedInitializer(std::size_t width, const std::string &space,
                          const AddressTable &addresses) {
        std::size_t depth = 0;
        bool braced = false;
        while (true) {
            // Outside braces the declaration ends at the newline, matching the
            // packer, so `uint8_t m1submapcount = 1` takes only its own value.
            if (!braced && skipBlanksToLineEnd()) return true;
            skipBlanks();
            if (position_ >= source_.size()) {
                if (!braced) return true;
                error_ = "unterminated typed initializer";
                return false;
            }
            const char byte = current();
            if (byte == '{') {
                braced = true;
                ++depth;
                ++position_;
                continue;
            }
            if (byte == '}') {
                --depth;
                ++position_;
                if (depth == 0) return true;
                continue;
            }
            if (byte == ';') {
                if (!braced) return true;
                ++position_;
                continue;
            }
            if (byte == ',') {
                ++position_;
                continue;
            }
            if (isDigit(byte)) {
                const std::optional<uint32_t> value = readNumber();
                if (!value) return false;
                pushBigEndian(*value, width);
                continue;
            }
            if (isIdentifierStart(byte)) {
                const std::string name = readQualifiedIdentifier();
                const std::optional<uint32_t> value = resolve(addresses, name, space);
                if (!value) {
                    error_ = "unresolved symbolic offset " + name;
                    return false;
                }
                pushBigEndian(*value, width);
                continue;
            }
            error_ = std::string("unexpected '") + byte + "' in typed initializer";
            return false;
        }
    }

    void pushBigEndian(uint32_t value, std::size_t width) {
        for (std::size_t index = width; index >= 1; --index) {
            bytes_.push_back(static_cast<uint8_t>((value >> ((index - 1) * 8)) & 0xff));
        }
    }

    static std::size_t declarationWidth(const std::string &identifier) {
        if (identifier == "uint8_t") return 1;
        if (identifier == "uint24_t") return 3;
        if (identifier == "uint32_t") return 4;
        return 0;
    }

    const std::string &source_;
    std::size_t position_ = 0;
    std::vector<uint8_t> bytes_;
    std::string error_;
    bool found_ = false;
};

std::string describeMismatch(const std::vector<uint8_t> &expected, const std::string &image,
                             uint32_t address) {
    std::ostringstream detail;
    detail << expected.size() << " bytes at 0x" << std::hex << std::uppercase << address
           << std::dec << " differ:";
    std::size_t reported = 0;
    for (std::size_t index = 0; index < expected.size() && reported < 4; ++index) {
        const auto actual = static_cast<uint8_t>(image[address + index]);
        if (actual == expected[index]) continue;
        detail << " [+" << index << "] image=" << static_cast<int>(actual)
               << " source=" << static_cast<int>(expected[index]);
        ++reported;
    }
    return detail.str();
}

// Byte parity alone cannot detect a generator dropping authored properties
// from both the raw source and the image. Pin real map cells independently
// of their generated values, just as the device tile suite does.
void checkAuthoredTiles(Report &report, const std::vector<uint8_t> &source,
                       const std::string &image, uint32_t address) {
    struct Cell {
        const char *name;
        uint16_t x, y, gid;
        uint8_t properties;
    };
    const Cell cells[] = {
        {"north floor", 12, 6, 275, TileProps::PROP_WALKABLE},
        {"spawn floor", 12, 7, 275, TileProps::PROP_WALKABLE},
        {"west wall", 11, 7, 262, 0},
        {"east wall", 13, 7, 261, 0},
    };
    for (const Cell &cell : cells) {
        const std::string label = std::string("authored tile ") + cell.name;
        const std::size_t offset =
            (static_cast<std::size_t>(cell.y) * Chunk::MAP_WIDTH_TILES + cell.x) * 2;
        if (offset + 2 > source.size() || address + offset + 2 > image.size()) {
            report.fail(label, "tile word runs past raw map or packed image");
            continue;
        }
        const uint16_t raw = source[offset] | (uint16_t(source[offset + 1]) << 8);
        const uint16_t packed = static_cast<uint8_t>(image[address + offset]) |
            (uint16_t(static_cast<uint8_t>(image[address + offset + 1])) << 8);
        const uint16_t authored = TileProps::packTile(cell.gid, cell.properties);
        if (raw == packed && TileProps::tileId(packed) == cell.gid &&
            TileProps::tileProps(packed) == cell.properties) {
            report.pass();
        } else {
            std::ostringstream detail;
            detail << "(" << cell.x << ',' << cell.y << ") expected=0x" << std::hex
                   << authored << " raw=0x" << raw << " packed=0x" << packed;
            report.fail(label, detail.str());
        }
    }
}

} // namespace

int main(int argc, char **argv) {
    const std::string root = argc > 1 ? argv[1] : ".";
    const std::string layoutPath = argc > 2 ? argv[2] : root + "/fxlayout.toml";
    const std::string headerPath = argc > 3 ? argv[3] : root + "/src/fxdata.h";
    const std::string imagePath = argc > 4 ? argv[4] : root + "/dist/fxdata-data.bin";

    const auto layout = readFile(layoutPath);
    const auto header = readFile(headerPath);
    const auto image = readFile(imagePath);
    for (const auto &[path, contents] : {std::pair{layoutPath, &layout},
                                         std::pair{headerPath, &header},
                                         std::pair{imagePath, &image}}) {
        if (!*contents) {
            std::cerr << "generated-libs: cannot read " << path << "\nremedy: run make gen\n";
            return 2;
        }
    }

    const AddressTable addresses = parseAddresses(*header);
    const std::vector<Entry> entries = parseLayout(*layout);
    if (addresses.qualified.empty() || entries.empty()) {
        std::cerr << "generated-libs: parsed " << addresses.qualified.size() << " addresses and "
                  << entries.size() << " entries; expected both to be non-empty\n";
        return 2;
    }

    Report report;
    std::size_t images = 0;
    for (const Entry &entry : entries) {
        const std::string label = "entry " + entry.key();
        if (entry.kind == "image") {
            // Image entries are re-encoded from PNG; their placement is covered
            // by the packed-image SHA baseline in pack-parity_test.sh.
            ++images;
            continue;
        }

        // An entry's own address is always its exact qualified name; the
        // first-declaration alias applies only to references inside tables.
        const auto published = addresses.qualified.find(entry.key());
        if (published == addresses.qualified.end()) {
            report.fail(label, "no uint24_t address published in " + headerPath);
            continue;
        }
        const uint32_t address = published->second;

        std::vector<uint8_t> expected;
        if (entry.kind == "raw") {
            const auto contents = readFile(root + "/" + entry.path);
            if (!contents) {
                report.fail(label, "raw source " + entry.path + " is missing");
                continue;
            }
            expected.assign(contents->begin(), contents->end());
        } else {
            const auto contents = readFile(root + "/" + entry.path);
            if (!contents) {
                report.fail(label, entry.kind + " source " + entry.path + " is missing");
                continue;
            }
            Decoder decoder(*contents);
            const auto decoded = entry.kind == "carray"
                                     ? decoder.carrayBytes(entry.symbol)
                                     : decoder.symbolBytes(entry.symbol, entry.space, addresses);
            if (!decoded) {
                report.fail(label, "decoding " + entry.path + " failed: " + decoder.error());
                continue;
            }
            expected = *decoded;
        }

        if (expected.empty()) {
            report.fail(label, "decoded zero bytes from " + entry.path);
            continue;
        }
        if (address + expected.size() > image->size()) {
            report.fail(label, "entry runs past the end of the packed image");
            continue;
        }
        if (entry.kind == "raw" && entry.key() == "raw_map_data") {
            checkAuthoredTiles(report, expected, *image, address);
        }
        bool matches = true;
        for (std::size_t index = 0; index < expected.size(); ++index) {
            if (static_cast<uint8_t>((*image)[address + index]) == expected[index]) continue;
            matches = false;
            break;
        }
        if (matches) {
            report.pass();
            continue;
        }
        report.fail(label, describeMismatch(expected, *image, address));
    }

    checkArenaCatalog<Decoder>(report, root, addresses, *image);

    std::cout << "generated-libs: " << images << " image entries covered by pack parity\n";
    return report.finish("generated-libs");
}
