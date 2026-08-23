#pragma once

#include <stdint.h>

constexpr uint16_t JOURNAL_RECORD_BYTES = 8;
constexpr uint16_t JOURNAL_CAPACITY = 4096 / JOURNAL_RECORD_BYTES;

struct JournalRecord {
    uint8_t seq;
    uint8_t op;
    uint8_t slot;
    uint8_t payload[4];
    uint8_t check;
};

static_assert(sizeof(JournalRecord) == JOURNAL_RECORD_BYTES,
              "JournalRecord must match its on-flash representation");

bool journalEncode(const JournalRecord &in, uint8_t out[JOURNAL_RECORD_BYTES]);
bool journalDecode(const uint8_t in[JOURNAL_RECORD_BYTES], JournalRecord &out);
uint16_t journalCount();
bool journalAppend(const JournalRecord &in);
uint16_t journalReplay(void (*apply)(const JournalRecord &));
bool journalFull();
void journalErase();
