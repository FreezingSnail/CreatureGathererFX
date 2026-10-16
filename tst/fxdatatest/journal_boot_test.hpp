#pragma once

#include <string.h>

#include "fxtest.hpp"
#include "src/fxdata.h"
#include "src/save/Compaction.hpp"
#include "src/save/FlashBackend.hpp"
#include "src/save/Journal.hpp"

namespace journal_boot_test {

inline void eraseSave()
{
    flash.eraseSector(16);
    FX::waitWhileBusy();
    flash.eraseSector(32);
    FX::waitWhileBusy();
    flash.eraseSector(48);
    FX::waitWhileBusy();
    storeFresh();
}

inline bool saveTo(SaveFile &state, bool stopBeforeJournalCleanup = false)
{
    saveBegin();
    SaveStep result = SaveStep::Idle;
    for (uint16_t i = 0; i < 256; ++i) {
        result = saveStepAdvance(state);
        FX::waitWhileBusy();
        if (result == SaveStep::Failed) return false;
        if (stopBeforeJournalCleanup && result == SaveStep::EraseJournal)
            return true;
        if (result == SaveStep::Done) return !stopBeforeJournalCleanup;
    }
    return false;
}

inline void run(FxTest &test)
{
    uint8_t bytes[JOURNAL_RECORD_BYTES];
    memset(bytes, 0xff, sizeof(bytes));
    JournalRecord decoded = {};
    test.expectEq(journalDecode(bytes, decoded), false, F("blank record has no commit marker"));
    bytes[3] = 0x27;
    test.expectEq(journalDecode(bytes, decoded), false, F("dirty uncommitted record is rejected"));
    JournalRecord authored = {};
    authored.seq = 19;
    authored.op = static_cast<uint8_t>(LogOp::StoreAdd);
    authored.slot = 0x1234;
    authored.payload[0] = 0x5a;
    uint8_t committed[JOURNAL_RECORD_BYTES];
    journalEncode(authored, committed);
    test.expectEq(journalDecode(committed, decoded), true, F("committed record decodes"));
    test.expectEq(decoded.slot, authored.slot, F("decoded slot matches"));
    test.expectEq(decoded.payload[0], authored.payload[0], F("decoded payload matches"));
    committed[14] ^= 1;
    test.expectEq(journalDecode(committed, decoded), false, F("bad checksum is rejected"));

    JournalRecord unbooted = {};
    unbooted.op = 99;
    test.expectEq(journalAppend(unbooted), false,
                  F("uninitialized production append fails closed"));

    eraseSave();
    SaveFile state = {};
    memset(state.partySlots, 255, sizeof(state.partySlots));
    test.expectEq(storeBoot(state), false, F("empty store starts fresh"));
    test.expectEq(storeBootFailed(), false, F("fresh store is playable"));
    test.expectEq(journalLength(), 0, F("fresh erase initializes tail"));

    test.expectEq(saveTo(state), true, F("initial checkpoint commits"));
    test.expectEq(storeBoot(state), true, F("checkpoint boot replays journal"));
    test.expectEq(journalLength(), 0, F("checkpoint replay initializes tail"));

    const StoreRecord record = {0, 27, 2, {0, 0, 0}, 0};
    test.expectEq(storeWrite(0, record), true, F("append after boot"));
    test.expectEq(storeBoot(state), true, F("boot restores first journal entry"));
    StoreRecord restored = {};
    test.expectEq(storeRead(0, restored), true, F("replayed slot is readable"));
    test.expectEq(restored.exp, 27, F("replayed record payload matches"));
    test.expectEq(journalLength(), 1, F("valid prefix tail is one"));

    test.expectEq(storeWrite(1, record), true, F("append uses replayed tail"));
    test.expectEq(journalLength(), 2, F("append advances replayed tail"));

    uint8_t torn[15];
    memset(torn, 0x5a, sizeof(torn));
    flash.writeBytes(save_log + 2 * JOURNAL_RECORD_BYTES, torn, sizeof(torn));
    test.expectEq(storeBoot(state), true, F("boot accepts prefix before torn tail"));
    test.expectEq(journalLength(), 2, F("replay stops at torn tail"));
    test.expectEq(storeWrite(2, record), false, F("torn nonblank tail is not overwritten"));

    test.expectEq(saveTo(state, true), true, F("checkpoint reaches pending cleanup"));
    test.expectEq(storeBoot(state), true, F("boot retries pending journal cleanup"));
    test.expectEq(journalLength(), 0, F("cleanup replay starts at empty tail"));
    test.expectEq(storeRead(0, restored), true, F("cleanup checkpoint retains prefix record"));
    test.expectEq(storeRead(1, restored), true, F("cleanup checkpoint retains second record"));

    JournalRecord unknown = {};
    unknown.op = 99;
    for (uint16_t i = 0; i < JOURNAL_CAPACITY; ++i)
        test.expectEq(journalAppend(unknown), true, F("journal represents each capacity slot"));
    test.expectEq(journalLength(), 256, F("uint16 tail represents full capacity"));
    test.expectEq(journalAppend(unknown), false, F("full journal refuses append"));
    test.expectEq(storeBoot(state), true, F("full valid journal replays"));
    test.expectEq(journalLength(), 256, F("full replay preserves tail 256"));
    test.expectEq(journalFull(), true, F("full tail reports full"));
    test.expectEq(journalAppend(unknown), false, F("full replay still refuses append"));
}

} // namespace journal_boot_test

