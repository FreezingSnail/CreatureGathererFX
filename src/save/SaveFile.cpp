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
    SaveFile candidate = {};
    if (!FX::loadGameState(reinterpret_cast<uint8_t *>(&candidate), sizeof(candidate)) ||
        candidate.version != SAVE_VERSION ||
        candidate.checksum != saveFileChecksum(candidate)) {
        return false;
    }

    // A failed load must not erase a caller's live/new-game RAM state.
    out = candidate;
    return true;
}
