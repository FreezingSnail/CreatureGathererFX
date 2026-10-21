// Permanent host test for the Creature Gatherer FX music streams.
//
// Contract under test: every row of data/music/catalog.tsv names a packaged
// stream whose file has the exact catalogued size and is a structurally valid
// 0xC1 streaming-loop compact stream, and whose music_* symbol is published by
// the generated src/fxdata.h. The append-only layout means the addresses must
// also be monotonic and non-overlapping in catalog order.
//
// Stream framing (tools/fx_pack/README.md in the ATMlib2FX repo):
//   byte 0        0xC1
//   u24 LE        loop_start_rel (record boundary, < stream length)
//   records       <len> <gap varint> <bitmap> (per set channel <glen> <bytes>)*
//   0x00          END, the loop boundary, at the final stream byte
//
// Usage: music_catalog_test [repo-root]

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <iterator>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

namespace {

constexpr uint8_t kStreamMagicLoop = 0xC1;
constexpr size_t kLoopHeader = 4;
constexpr uint8_t kRing = 64;

struct Expected {
    const char *theme;
    const char *symbol;
    unsigned bytes;
};

constexpr Expected kExpected[] = {
    {"overworld", "music_overworld", 6328},
    {"combat", "music_combat", 5772},
    {"victory", "music_victory", 1338},
    {"defeat", "music_defeat", 674},
    {"opening", "music_opening", 7208},
    {"village", "music_village", 1660},
    {"forest", "music_forest", 6204},
    {"coast", "music_coast", 2964},
    {"marsh", "music_marsh", 1452},
    {"ruins", "music_ruins", 7056},
    {"exploration", "music_exploration", 1928},
};

struct Row {
    std::string theme;
    std::string symbol;
    unsigned bytes = 0;
    std::string file;
};

class Report {
  public:
    void check(bool ok, const std::string &label, const std::string &detail = std::string()) {
        if (ok) {
            ++passes_;
            return;
        }
        failures_.push_back({label, detail});
    }

    int finish() const {
        for (const auto &failure : failures_) {
            std::cout << "FAIL " << failure.first << "\n     " << failure.second << "\n";
        }
        std::cout << "music catalog: " << passes_ << " passed, " << failures_.size() << " failed\n";
        return failures_.empty() ? 0 : 1;
    }

  private:
    std::vector<std::pair<std::string, std::string>> failures_;
    unsigned passes_ = 0;
};

bool readText(const std::string &path, std::string &out) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return false;
    std::ostringstream buffer;
    buffer << in.rdbuf();
    out = buffer.str();
    return true;
}

bool readBytes(const std::string &path, std::vector<uint8_t> &out) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return false;
    out.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
    return true;
}

std::string join(const std::string &root, const std::string &relative) {
    if (root.empty() || root == ".") return relative;
    return root + "/" + relative;
}

std::vector<std::string> splitTabs(const std::string &line) {
    std::vector<std::string> fields;
    size_t start = 0;
    while (true) {
        const size_t tab = line.find('\t', start);
        fields.push_back(line.substr(start, tab - start));
        if (tab == std::string::npos) break;
        start = tab + 1;
    }
    return fields;
}

bool parseCatalog(const std::string &text, std::vector<Row> &rows, std::string &error) {
    std::istringstream in(text);
    std::string line;
    bool header = true;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) continue;
        if (header) {
            header = false;
            if (line != "theme\tsymbol\tbytes\tfile") {
                error = "unexpected header: " + line;
                return false;
            }
            continue;
        }
        const auto fields = splitTabs(line);
        if (fields.size() != 4) {
            error = "expected 4 tab-separated fields in: " + line;
            return false;
        }
        Row row;
        row.theme = fields[0];
        row.symbol = fields[1];
        row.file = fields[3];
        try {
            row.bytes = static_cast<unsigned>(std::stoul(fields[2]));
        } catch (...) {
            error = "non-numeric byte count in: " + line;
            return false;
        }
        rows.push_back(row);
    }
    return true;
}

// Decodes the compact gap varint. `pos` advances past the field.
bool decodeGap(const std::vector<uint8_t> &data, size_t &pos, size_t limit, uint32_t &gap) {
    if (pos >= limit) return false;
    const uint8_t b0 = data[pos];
    if (b0 < 0x80) {
        gap = b0;
        pos += 1;
        return true;
    }
    if (b0 != 0xFF) {
        if (pos + 2 > limit) return false;
        gap = (static_cast<uint32_t>(b0 & 0x7F) << 8) | data[pos + 1];
        pos += 2;
        return true;
    }
    if (pos + 3 > limit) return false;
    gap = (static_cast<uint32_t>(data[pos + 1]) << 8) | data[pos + 2];
    pos += 3;
    return true;
}

// Validates the 0xC1 header and record framing. Returns an empty string on
// success, otherwise a diagnostic naming the first structural defect.
std::string validateStream(const std::string &theme, const std::vector<uint8_t> &data) {
    if (data.size() < kLoopHeader + 1) {
        return theme + ": stream is " + std::to_string(data.size()) + " bytes, shorter than the header";
    }
    if (data[0] != kStreamMagicLoop) {
        char buffer[64];
        std::snprintf(buffer, sizeof(buffer), "%s: magic 0x%02X, expected 0xC1", theme.c_str(), data[0]);
        return buffer;
    }
    const uint32_t loopStart = static_cast<uint32_t>(data[1]) |
                               (static_cast<uint32_t>(data[2]) << 8) |
                               (static_cast<uint32_t>(data[3]) << 16);
    if (loopStart < kLoopHeader || loopStart >= data.size()) {
        char buffer[96];
        std::snprintf(buffer, sizeof(buffer), "%s: loop_start_rel 0x%04X outside [4, %zu)",
                      theme.c_str(), static_cast<unsigned>(loopStart), data.size());
        return buffer;
    }

    std::vector<size_t> boundaries{kLoopHeader};
    size_t pos = kLoopHeader;
    bool loopGap = false;
    while (pos < data.size()) {
        const uint8_t reclen = data[pos];
        if (reclen == 0) {
            boundaries.push_back(pos + 1);
            if (pos + 1 != data.size()) {
                return theme + ": END at offset " + std::to_string(pos) + " is not the final byte";
            }
            if (std::find(boundaries.begin(), boundaries.end(), loopStart) == boundaries.end()) {
                return theme + ": loop_start_rel is not a record boundary";
            }
            if (!loopGap) {
                return theme + ": loop region has no non-zero gap";
            }
            return std::string();
        }
        if (reclen < 3) {
            return theme + ": record length " + std::to_string(reclen) + " < 3 at offset " + std::to_string(pos);
        }
        if (reclen > kRing) {
            return theme + ": record length " + std::to_string(reclen) + " > ring 64 at offset " +
                   std::to_string(pos);
        }
        const size_t end = pos + reclen;
        if (end > data.size()) {
            return theme + ": record at offset " + std::to_string(pos) + " runs past the stream end";
        }
        size_t p = pos + 1;
        uint32_t gap = 0;
        if (!decodeGap(data, p, end, gap)) {
            return theme + ": truncated gap varint at offset " + std::to_string(pos);
        }
        if (pos >= loopStart && gap != 0) loopGap = true;
        if (p >= end) {
            return theme + ": record at offset " + std::to_string(pos) + " has no bitmap";
        }
        const uint8_t bitmap = data[p++];
        for (uint8_t channel = 0; channel < 4; ++channel) {
            if (!(bitmap & (1u << channel))) continue;
            if (p >= end) {
                return theme + ": record at offset " + std::to_string(pos) + " missing group length";
            }
            const uint8_t glen = data[p++];
            if (glen == 0) {
                return theme + ": record at offset " + std::to_string(pos) + " has glen 0";
            }
            if (p + glen > end) {
                return theme + ": group length overflows record at offset " + std::to_string(pos);
            }
            p += glen;
        }
        if (p != end) {
            return theme + ": record at offset " + std::to_string(pos) + " has " +
                   std::to_string(end - p) + " trailing bytes";
        }
        boundaries.push_back(end);
        pos = end;
    }
    return theme + ": stream has no END record";
}

} // namespace

int main(int argc, char **argv) {
    const std::string root = argc > 1 ? argv[1] : ".";
    Report report;

    std::string catalogText;
    const bool catalogRead = readText(join(root, "data/music/catalog.tsv"), catalogText);
    report.check(catalogRead, "catalog.tsv readable", "data/music/catalog.tsv could not be read");

    std::vector<Row> rows;
    std::string catalogError;
    if (catalogRead) {
        const bool parsed = parseCatalog(catalogText, rows, catalogError);
        report.check(parsed, "catalog.tsv parsed", catalogError);
    }
    report.check(rows.size() == std::size(kExpected), "catalog row count",
                 "expected " + std::to_string(std::size(kExpected)) + " rows, found " +
                     std::to_string(rows.size()));

    std::string headerText;
    const bool headerRead = readText(join(root, "src/fxdata.h"), headerText);
    report.check(headerRead, "fxdata.h readable", "src/fxdata.h could not be read");

    std::string catalogHeaderText;
    const bool catalogHeaderRead = readText(join(root, "src/audio/MusicCatalog.hpp"), catalogHeaderText);
    report.check(catalogHeaderRead, "MusicCatalog.hpp readable", "src/audio/MusicCatalog.hpp could not be read");

    const size_t count = std::min(rows.size(), std::size(kExpected));
    uint32_t previousAddress = 0;
    for (size_t i = 0; i < count; ++i) {
        const Row &row = rows[i];
        const Expected &expected = kExpected[i];
        const std::string prefix = row.theme;

        report.check(row.theme == expected.theme, prefix + ": theme order",
                     "expected " + std::string(expected.theme) + ", found " + row.theme);
        report.check(row.symbol == expected.symbol, prefix + ": symbol",
                     "expected " + std::string(expected.symbol) + ", found " + row.symbol);
        report.check(row.bytes == expected.bytes, prefix + ": catalog bytes",
                     "expected " + std::to_string(expected.bytes) + ", found " + std::to_string(row.bytes));

        std::vector<uint8_t> data;
        const bool read = readBytes(join(root, row.file), data);
        report.check(read, prefix + ": stream readable", row.file + " could not be read");
        if (!read) continue;

        report.check(data.size() == row.bytes, prefix + ": stream size",
                     "expected " + std::to_string(row.bytes) + ", found " + std::to_string(data.size()));
        const std::string defect = validateStream(row.theme, data);
        report.check(defect.empty(), prefix + ": stream structure", defect);

        const std::regex symbolPattern("constexpr\\s+uint24_t\\s+" + row.symbol +
                                       "\\s*=\\s*0x([0-9A-Fa-f]+)\\s*;");
        std::smatch match;
        const bool published = headerRead && std::regex_search(headerText, match, symbolPattern);
        report.check(published, prefix + ": symbol published", row.symbol + " missing from src/fxdata.h");
        if (published) {
            const uint32_t address = static_cast<uint32_t>(std::stoul(match[1].str(), nullptr, 16));
            report.check(address >= previousAddress, prefix + ": append-only address order",
                         row.symbol + " overlaps or precedes the previous theme");
            previousAddress = address + row.bytes;
        }

        const std::string tableRow = "{" + row.symbol + ", " + std::to_string(row.bytes) + "}";
        report.check(catalogHeaderRead && catalogHeaderText.find(tableRow) != std::string::npos,
                     prefix + ": MusicCatalog entry",
                     "src/audio/MusicCatalog.hpp is missing " + tableRow);
    }

    return report.finish();
}
