#include "Journal.hpp"

#include <stddef.h>
#include <string.h>

#include "FlashBackend.hpp"

#ifdef TEST
constexpr uint32_t JOURNAL_SAVE_LOG = 4096;
#else
#include "../fxdata.h"
constexpr uint24_t JOURNAL_SAVE_LOG = save_log;
#endif

namespace {
constexpr uint16_t JOURNAL_WINDOW_BYTES = 32;
constexpr uint8_t JOURNAL_RECORDS_PER_WINDOW =
    JOURNAL_WINDOW_BYTES / JOURNAL_RECORD_BYTES;
constexpr uint16_t JOURNAL_SECTOR_PAGE = 16;
constexpr uint16_t JOURNAL_UNINITIALIZED = 0xffff;
uint16_t journalTail = JOURNAL_UNINITIALIZED;

uint8_t journalCheck(const uint8_t bytes[JOURNAL_RECORD_BYTES])
{
    // The check byte is at 14: choosing ~xor(0..13) makes xor(0..14)
    // equal 0xff. Byte 15 is padding and is outside the checksum.
    uint8_t check = 0;
    for (uint8_t i = 0; i < offsetof(JournalRecord, check); ++i) {
        check ^= bytes[i];
    }
    return static_cast<uint8_t>(~check);
}

bool journalBlank(const uint8_t bytes[JOURNAL_RECORD_BYTES])
{
    for (uint8_t i = 0; i < JOURNAL_RECORD_BYTES; ++i) {
        if (bytes[i] != 0xff) {
            return false;
        }
    }
    return true;
}

bool journalKnownOp(uint8_t op)
{
    return op >= static_cast<uint8_t>(LogOp::StoreAdd) &&
           op <= static_cast<uint8_t>(LogOp::PartyAssign);
}
} // namespace

bool journalEncode(const JournalRecord &in, uint8_t out[JOURNAL_RECORD_BYTES])
{
    out[0] = in.seq;
    out[1] = in.op;
    out[2] = static_cast<uint8_t>(in.slot);
    out[3] = static_cast<uint8_t>(in.slot >> 8);
    memcpy(out + 4, in.payload, sizeof(in.payload));
    out[14] = journalCheck(out);
    out[15] = 0xff;
    return true;
}

bool journalDecode(const uint8_t in[JOURNAL_RECORD_BYTES], JournalRecord &out)
{
    if (journalBlank(in) || in[14] != journalCheck(in)) {
        return false;
    }

    out.seq = in[0];
    out.op = in[1];
    out.slot = static_cast<uint16_t>(in[2]) |
               static_cast<uint16_t>(in[3] << 8);
    memcpy(out.payload, in + 4, sizeof(out.payload));
    out.check = in[14];
    out.pad = in[15];
    return true;
}

uint16_t journalCount()
{
    uint8_t window[JOURNAL_WINDOW_BYTES];
    uint16_t count = 0;

    for (uint16_t offset = 0; offset < JOURNAL_CAPACITY * JOURNAL_RECORD_BYTES;
         offset += JOURNAL_WINDOW_BYTES) {
        flash.readBytes(JOURNAL_SAVE_LOG + offset, window, sizeof(window));
        for (uint8_t record = 0; record < JOURNAL_RECORDS_PER_WINDOW; ++record) {
            JournalRecord decoded;
            if (!journalDecode(window + record * JOURNAL_RECORD_BYTES, decoded)) {
                return count;
            }
            ++count;
        }
    }
    return count;
}

void journalInit()
{
    journalTail = journalCount();
}

bool journalAppend(const JournalRecord &in)
{
    if (journalTail == JOURNAL_UNINITIALIZED) {
        journalInit();
    }
    if (journalTail == JOURNAL_CAPACITY) {
        return false;
    }

    uint8_t bytes[JOURNAL_RECORD_BYTES];
    const uint32_t address = JOURNAL_SAVE_LOG + (static_cast<uint32_t>(journalTail) << 4);
    flash.readBytes(address, bytes, sizeof(bytes));
    if (!journalBlank(bytes)) {
        return false; // A torn tail cannot be overwritten by NOR flash.
    }
    journalEncode(in, bytes);
    flash.writeBytes(address, bytes, sizeof(bytes));
    while (flash.busy()) {
    }
    ++journalTail;
    return true;
}

uint16_t journalReplay(void (*apply)(const JournalRecord &))
{
    uint8_t window[JOURNAL_WINDOW_BYTES];
    uint16_t count = 0;

    for (uint16_t offset = 0; offset < JOURNAL_CAPACITY * JOURNAL_RECORD_BYTES;
         offset += JOURNAL_WINDOW_BYTES) {
        flash.readBytes(JOURNAL_SAVE_LOG + offset, window, sizeof(window));
        for (uint8_t record = 0; record < JOURNAL_RECORDS_PER_WINDOW; ++record) {
            JournalRecord decoded;
            if (!journalDecode(window + record * JOURNAL_RECORD_BYTES, decoded)) {
                return count;
            }
            if (journalKnownOp(decoded.op)) {
                apply(decoded);
            }
            ++count;
        }
    }
    return count;
}

bool journalFull()
{
    if (journalTail == JOURNAL_UNINITIALIZED) {
        journalInit();
    }
    return journalTail == JOURNAL_CAPACITY;
}

void journalErase()
{
    flash.eraseSector(JOURNAL_SECTOR_PAGE);
    journalTail = 0;
}
