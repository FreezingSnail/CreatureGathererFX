#include "Journal.hpp"

#include <string.h>

#include "FlashBackend.hpp"

#ifdef TEST
constexpr uint32_t JOURNAL_SAVE_LOG = 4096;
#else
#include "../fxdata.h"
constexpr uint24_t JOURNAL_SAVE_LOG = save_log;
#endif

namespace {
constexpr uint16_t JOURNAL_PAGE_BYTES = 256;
constexpr uint16_t JOURNAL_RECORDS_PER_PAGE =
    JOURNAL_PAGE_BYTES / JOURNAL_RECORD_BYTES;
constexpr uint16_t JOURNAL_SECTOR_PAGE = 16;

uint8_t journalCheck(const uint8_t bytes[JOURNAL_RECORD_BYTES - 1])
{
    uint8_t check = 0;
    for (uint8_t i = 0; i < JOURNAL_RECORD_BYTES - 1; ++i) {
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
} // namespace

bool journalEncode(const JournalRecord &in, uint8_t out[JOURNAL_RECORD_BYTES])
{
    out[0] = in.seq;
    out[1] = in.op;
    out[2] = in.slot;
    memcpy(out + 3, in.payload, sizeof(in.payload));
    out[7] = journalCheck(out);
    return true;
}

bool journalDecode(const uint8_t in[JOURNAL_RECORD_BYTES], JournalRecord &out)
{
    if (journalBlank(in) || in[7] != journalCheck(in)) {
        return false;
    }

    out.seq = in[0];
    out.op = in[1];
    out.slot = in[2];
    memcpy(out.payload, in + 3, sizeof(out.payload));
    out.check = in[7];
    return true;
}

uint16_t journalCount()
{
    uint8_t page[JOURNAL_PAGE_BYTES];
    uint16_t count = 0;

    for (uint16_t offset = 0; offset < JOURNAL_CAPACITY * JOURNAL_RECORD_BYTES;
         offset += JOURNAL_PAGE_BYTES) {
        flash.readBytes(JOURNAL_SAVE_LOG + offset, page, sizeof(page));
        for (uint8_t record = 0; record < JOURNAL_RECORDS_PER_PAGE; ++record) {
            JournalRecord decoded;
            if (!journalDecode(page + record * JOURNAL_RECORD_BYTES, decoded)) {
                return count;
            }
            ++count;
        }
    }
    return count;
}

bool journalAppend(const JournalRecord &in)
{
    const uint16_t count = journalCount();
    if (count == JOURNAL_CAPACITY) {
        return false;
    }

    uint8_t bytes[JOURNAL_RECORD_BYTES];
    journalEncode(in, bytes);
    flash.writeBytes(JOURNAL_SAVE_LOG + count * JOURNAL_RECORD_BYTES, bytes, sizeof(bytes));
    while (flash.busy()) {
    }
    return true;
}

uint16_t journalReplay(void (*apply)(const JournalRecord &))
{
    uint8_t page[JOURNAL_PAGE_BYTES];
    uint16_t count = 0;

    for (uint16_t offset = 0; offset < JOURNAL_CAPACITY * JOURNAL_RECORD_BYTES;
         offset += JOURNAL_PAGE_BYTES) {
        flash.readBytes(JOURNAL_SAVE_LOG + offset, page, sizeof(page));
        for (uint8_t record = 0; record < JOURNAL_RECORDS_PER_PAGE; ++record) {
            JournalRecord decoded;
            if (!journalDecode(page + record * JOURNAL_RECORD_BYTES, decoded)) {
                return count;
            }
            apply(decoded);
            ++count;
        }
    }
    return count;
}

bool journalFull()
{
    return journalCount() == JOURNAL_CAPACITY;
}

void journalErase()
{
    flash.eraseSector(JOURNAL_SECTOR_PAGE);
}
