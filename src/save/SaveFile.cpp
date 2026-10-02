#include "SaveFile.hpp"

#include <string.h>

#include "FlashBackend.hpp"

namespace {
// Pre-Effect-narrowing SaveFile v1 was 157 bytes on AVR. Treat it as an
// incompatible save and skip it; jp8.2.6 owns the next schema migration.
constexpr uint16_t LEGACY_SAVE_V1_AVR_BYTES = 157;
}

uint16_t saveFileChecksum(const SaveFile &in)
{
    const uint8_t *bytes = reinterpret_cast<const uint8_t *>(&in);
    uint16_t sum1 = 0;
    uint16_t sum2 = 0;

    for (size_t i = 0; i < offsetof(SaveFile, checksum); ++i) {
        sum1 = static_cast<uint16_t>((sum1 + bytes[i]) % 255);
        sum2 = static_cast<uint16_t>((sum2 + sum1) % 255);
    }

    return static_cast<uint16_t>((sum2 << 8) | sum1);
}

void saveFileCommit(const SaveFile &in)
{
    SaveFile record = in;
    record.version = SAVE_VERSION;
    record.checksum = saveFileChecksum(record);
    uint8_t firstHeader[2];
    flash.readBytes(0, firstHeader, sizeof(firstHeader));
    const uint16_t firstSize = static_cast<uint16_t>(firstHeader[0] << 8) |
                               static_cast<uint16_t>(firstHeader[1]);
    if (firstSize == LEGACY_SAVE_V1_AVR_BYTES) {
        // The platform append helper cannot safely append a differently sized
        // record behind v1, so discard that sector before the first new save.
        flash.eraseSector(0);
    }
    FX::saveGameState(reinterpret_cast<const uint8_t *>(&record), sizeof(record));
}

bool saveFileLoad(SaveFile &out)
{
    constexpr uint16_t SAVE_SECTOR_BYTES = 4096;
    uint16_t addr = 0;
    bool found = false;
    SaveFile latest = {};

    while (addr + 2 <= SAVE_SECTOR_BYTES) {
        uint8_t header[2];
        flash.readBytes(addr, header, sizeof(header));
        const uint16_t size = static_cast<uint16_t>(header[0] << 8) |
                              static_cast<uint16_t>(header[1]);
        if (addr + 2 + size > SAVE_SECTOR_BYTES) {
            break;
        }

        if (size == LEGACY_SAVE_V1_AVR_BYTES) {
            uint8_t legacyVersion = 0;
            flash.readBytes(addr + 2, &legacyVersion, sizeof(legacyVersion));
            if (legacyVersion != SAVE_VERSION) {
                break;
            }
            addr = static_cast<uint16_t>(addr + 2 + size);
            continue;
        }
        if (size != sizeof(SaveFile)) {
            break;
        }

        SaveFile candidate = {};
        flash.readBytes(addr + 2, reinterpret_cast<uint8_t *>(&candidate), size);
        if (candidate.version == SAVE_VERSION &&
            candidate.checksum == saveFileChecksum(candidate)) {
            latest = candidate;
            found = true;
        }
        addr = static_cast<uint16_t>(addr + 2 + size);
    }

    if (!found) {
        return false;
    }
    out = latest;
    return true;
}
