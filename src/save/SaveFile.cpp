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
    memset(&out, 0, sizeof(out));
    if (!FX::loadGameState(reinterpret_cast<uint8_t *>(&out), sizeof(out)) ||
        out.version != SAVE_VERSION ||
        out.checksum != saveFileChecksum(out)) {
        memset(&out, 0, sizeof(out));
        return false;
    }
    return true;
}
