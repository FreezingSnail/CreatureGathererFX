#include "SaveFile.hpp"

#include <string.h>

#include "FlashBackend.hpp"

namespace {
// Pre-party-HP SaveFile v1 was 157 bytes on AVR. Treat it as an incompatible
// save and skip it; jp8.2.6 owns the next schema migration.
constexpr uint16_t LEGACY_SAVE_V1_AVR_BYTES = 157;
constexpr uint8_t SAVE_VALIDATION_BYTES = 8;

bool saveFileRecordValid(uint16_t recordAddr)
{
    uint8_t bytes[SAVE_VALIDATION_BYTES];
    uint16_t sum1 = 0;
    uint16_t sum2 = 0;
    uint8_t version = 0;
    constexpr uint16_t payloadBytes = offsetof(SaveFile, checksum);

    for (uint16_t offset = 0; offset < payloadBytes;
         offset += SAVE_VALIDATION_BYTES) {
        const uint8_t count = static_cast<uint8_t>(
            payloadBytes - offset < SAVE_VALIDATION_BYTES
                ? payloadBytes - offset
                : SAVE_VALIDATION_BYTES);
        flash.readBytes(recordAddr + offset, bytes, count);
        if (offset == 0) {
            version = bytes[0];
        }
        for (uint8_t i = 0; i < count; ++i) {
            const uint16_t next1 = static_cast<uint16_t>(sum1 + bytes[i]);
            sum1 = next1 >= 255 ? static_cast<uint16_t>(next1 - 255) : next1;
            const uint16_t next2 = static_cast<uint16_t>(sum2 + sum1);
            sum2 = next2 >= 255 ? static_cast<uint16_t>(next2 - 255) : next2;
        }
    }

    flash.readBytes(recordAddr + payloadBytes, bytes, 2);
    const uint16_t stored = static_cast<uint16_t>(bytes[0]) |
                            static_cast<uint16_t>(bytes[1] << 8);
    return version == SAVE_VERSION &&
           stored == static_cast<uint16_t>((sum2 << 8) | sum1);
}

bool latestValidRecordAddr(uint16_t &latestAddr)
{
    constexpr uint16_t SAVE_SECTOR_BYTES = 4096;
    uint16_t addr = 0;
    bool found = false;

    while (addr + 2 <= SAVE_SECTOR_BYTES) {
        uint8_t header[2];
        flash.readBytes(addr, header, sizeof(header));
        const uint16_t size = static_cast<uint16_t>(header[0] << 8) |
                              static_cast<uint16_t>(header[1]);
        if (addr + 2 + size > SAVE_SECTOR_BYTES) {
            break;
        }

        if (size == LEGACY_SAVE_V1_AVR_BYTES) {
            // Legacy records cannot validate against the v2 shape, but a
            // later current record in the append log remains loadable.
            addr = static_cast<uint16_t>(addr + 2 + size);
            continue;
        }
        if (size != sizeof(SaveFile)) {
            break;
        }

        if (saveFileRecordValid(static_cast<uint16_t>(addr + 2))) {
            latestAddr = static_cast<uint16_t>(addr + 2);
            found = true;
        }
        addr = static_cast<uint16_t>(addr + 2 + size);
    }
    return found;
}
}

uint16_t saveFileChecksum(const SaveFile &in)
{
    const uint8_t *bytes = reinterpret_cast<const uint8_t *>(&in);
    uint16_t sum1 = 0;
    uint16_t sum2 = 0;

    for (size_t i = 0; i < offsetof(SaveFile, checksum); ++i) {
        const uint16_t next1 = static_cast<uint16_t>(sum1 + bytes[i]);
        sum1 = next1 >= 255 ? static_cast<uint16_t>(next1 - 255) : next1;
        const uint16_t next2 = static_cast<uint16_t>(sum2 + sum1);
        sum2 = next2 >= 255 ? static_cast<uint16_t>(next2 - 255) : next2;
    }

    return static_cast<uint16_t>((sum2 << 8) | sum1);
}

void saveFileCommit(const SaveFile &in)
{
    SaveFile record = in;
    record.version = SAVE_VERSION;
    record.checksum = saveFileChecksum(record);
    saveFileCommitPrepared(record);
}

void saveFileCommitPrepared(const SaveFile &record)
{
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
    uint16_t latestAddr = 0;
    if (!latestValidRecordAddr(latestAddr)) {
        return false;
    }
    flash.readBytes(latestAddr, reinterpret_cast<uint8_t *>(&out), sizeof(out));
    return true;
}

bool saveFileLoadParty(SaveFile &state)
{
    uint16_t latestAddr = 0;
    if (!latestValidRecordAddr(latestAddr)) {
        return false;
    }
    flash.readBytes(latestAddr + offsetof(SaveFile, party),
                    reinterpret_cast<uint8_t *>(state.party), sizeof(state.party));
    flash.readBytes(latestAddr + offsetof(SaveFile, partyHP),
                    state.partyHP, sizeof(state.partyHP));
    return true;
}

bool saveFileMatchesStored(const SaveFile &expected)
{
    uint16_t latestAddr = 0;
    if (!latestValidRecordAddr(latestAddr)) {
        return false;
    }

    uint8_t bytes[SAVE_VALIDATION_BYTES];
    const uint8_t *wanted = reinterpret_cast<const uint8_t *>(&expected);
    for (uint16_t offset = 0; offset < sizeof(expected);
         offset += SAVE_VALIDATION_BYTES) {
        const uint8_t count = static_cast<uint8_t>(
            sizeof(expected) - offset < SAVE_VALIDATION_BYTES
                ? sizeof(expected) - offset
                : SAVE_VALIDATION_BYTES);
        flash.readBytes(latestAddr + offset, bytes, count);
        if (memcmp(bytes, wanted + offset, count) != 0) {
            return false;
        }
    }
    return true;
}
