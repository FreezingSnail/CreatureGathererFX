#include "SaveFile.hpp"
#include <string.h>
#include "Compaction.hpp"
#include "../item/ItemIds.hpp"

void saveFileEncode(const SaveFile &in, uint8_t offset, uint8_t *out, uint8_t count)
{
    for (uint8_t i = 0; i < count; ++i) {
        const uint8_t at = offset + i;
        out[i] = at < 2 ? static_cast<uint8_t>(in.playerLocation >> (8 * at))
                       : reinterpret_cast<const uint8_t *>(&in.flags)[at - 2];
    }
}
void saveFileDecode(const uint8_t *in, SaveFile &out)
{
    out.playerLocation = static_cast<uint16_t>(in[0]) | (static_cast<uint16_t>(in[1]) << 8);
    memcpy(out.flags, in + 2, 94);
}
__attribute__((noinline)) bool saveSnapshotByteValid(uint8_t offset, uint8_t byte)
{
    if (offset >= 27 && offset < 59) return byte <= item::COUNT_MAX;
    if ((offset >= 3 && offset < 10) || offset >= 60) return byte == 0;
    return true;
}
__attribute__((noinline)) bool savePartySlotsValid(const uint8_t slots[3])
{
    for (uint8_t i = 0; i < 3; ++i) {
        if (slots[i] != 255 && slots[i] >= 64) return false;
        for (uint8_t j = 0; j < i; ++j)
            if (slots[i] != 255 && slots[i] == slots[j]) return false;
    }
    return true;
}
bool saveFileValid(const SaveFile &in)
{
    if (!savePartySlotsValid(in.partySlots)) return false;
    const uint8_t *bytes = reinterpret_cast<const uint8_t *>(in.flags);
    for (uint8_t i = 2; i < 96; ++i)
        if (!saveSnapshotByteValid(i, bytes[i - 2])) return false;
    return true;
}
bool saveFileLoad(SaveFile &out) { return storeBoot(out); }
