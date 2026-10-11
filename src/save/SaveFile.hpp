#pragma once
#include <stdint.h>

constexpr uint8_t SAVE_VERSION = 3;
constexpr uint8_t PARTY_MAX = 3;
constexpr uint8_t SAVE_SNAPSHOT_BYTES = 96;

// Byte arrays have explicit wire offsets; location is encoded little-endian.
struct SaveFile {
    uint16_t playerLocation;
    uint8_t flags[8];
    uint8_t partySlots[3];
    uint8_t partyHP[3];
    uint8_t plantStages[8];
    uint8_t plantPairs[2];
    uint8_t plantTicker;
    uint8_t lures[24];
    uint8_t consumables[8];
    uint8_t keyItems[4];
    uint8_t future[33];
};
static_assert(sizeof(SaveFile) == SAVE_SNAPSHOT_BYTES, "Snapshot size is fixed");
void saveFileEncode(const SaveFile &in, uint8_t offset, uint8_t *out, uint8_t count);
void saveFileDecode(const uint8_t *in, SaveFile &out);
bool saveFileValid(const SaveFile &in);
bool saveSnapshotByteValid(uint8_t offset, uint8_t byte);
bool savePartySlotsValid(const uint8_t slots[3]);
bool saveFileLoad(SaveFile &out);
