#include "SaveFile.hpp"

#include <string.h>

#include "FlashBackend.hpp"

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
        const uint16_t size = static_cast<uint16_t>(header[0]) |
                              static_cast<uint16_t>(header[1] << 8);
        if (size != sizeof(SaveFile) || addr + 2 + size > SAVE_SECTOR_BYTES) {
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
