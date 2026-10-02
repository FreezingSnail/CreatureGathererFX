#pragma once

#include <stddef.h>
#include <stdint.h>

constexpr uint16_t JOURNAL_RECORD_BYTES = 16;
constexpr uint16_t JOURNAL_CAPACITY = 4096 / JOURNAL_RECORD_BYTES;

// The journal is for state too large to keep in RAM (store records and party
// assignments). Inventory, flags, plants, lure charges, and party HP remain
// in the SaveFile snapshot rather than consuming a log record per change.
enum class LogOp : uint8_t { StoreAdd = 2, StoreRemove = 3, PartyAssign = 4 };

struct JournalRecord {
    uint8_t seq;
    uint8_t op;
    uint16_t slot;
    uint8_t payload[10];
    uint8_t check;
    uint8_t pad;
};

static_assert(sizeof(JournalRecord) == JOURNAL_RECORD_BYTES,
              "JournalRecord must match its on-flash representation");
static_assert(offsetof(JournalRecord, slot) == 2, "Slot starts at byte 2");
static_assert(offsetof(JournalRecord, payload) == 4, "Payload starts at byte 4");
static_assert(offsetof(JournalRecord, check) == 14, "Check is byte 14");
static_assert(offsetof(JournalRecord, pad) == 15, "Pad is byte 15");
static_assert(JOURNAL_RECORD_BYTES * 16 == 256, "Sixteen records fit a page");
static_assert(JOURNAL_CAPACITY == 256, "Journal is one sector");

bool journalEncode(const JournalRecord &in, uint8_t out[JOURNAL_RECORD_BYTES]);
bool journalDecode(const uint8_t in[JOURNAL_RECORD_BYTES], JournalRecord &out);
void journalInit();
uint16_t journalCount();
bool journalAppend(const JournalRecord &in);
uint16_t journalReplay(void (*apply)(const JournalRecord &));
bool journalFull();
void journalErase();
