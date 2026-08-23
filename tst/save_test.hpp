#pragma once

#include <string.h>

#include "test.hpp"
#include "../src/save/Compaction.hpp"
#include "../src/save/FlashBackend.hpp"
#include "../src/save/Journal.hpp"
#include "../src/save/SaveFile.hpp"

namespace save_test_detail {
inline void advanceToDone(SaveFile &state)
{
    for (uint8_t i = 0; i < 16 && saveInProgress(); ++i) {
        saveStepAdvance(state);
    }
}

inline JournalRecord record(uint8_t value, uint8_t slot = 0, uint8_t op = 0)
{
    JournalRecord result = {};
    result.seq = value;
    result.op = op;
    result.slot = slot;
    result.payload[0] = value;
    result.payload[1] = static_cast<uint8_t>(value + 1);
    result.payload[2] = static_cast<uint8_t>(value + 2);
    result.payload[3] = static_cast<uint8_t>(value + 3);
    return result;
}

inline SaveFile state(uint16_t location, uint8_t fill)
{
    SaveFile result = {};
    result.playerLocation = location;
    memset(result.flags, fill, sizeof(result.flags));
    memset(result.party, fill, sizeof(result.party));
    memset(&result.plants, fill, sizeof(result.plants));
    memset(result.inventory, fill, sizeof(result.inventory));
    return result;
}

inline uint16_t replayOrder = 0;
inline SaveFile *replayState = nullptr;

inline void recordOrder(const JournalRecord &record)
{
    replayOrder = static_cast<uint16_t>(replayOrder * 10 + record.payload[0]);
}

inline void applyRecord(const JournalRecord &record)
{
    memcpy(&replayState->party[record.slot], record.payload, sizeof(record.payload));
}
} // namespace save_test_detail

inline void SaveFileRoundTripAndValidationTest(TestSuite &suite)
{
    Test test = Test(__func__);
    flashFakeReset();

    SaveFile original = save_test_detail::state(0x1234, 0x5a);
    saveFileCommit(original);
    original.version = SAVE_VERSION;
    original.checksum = saveFileChecksum(original);

    SaveFile loaded = {};
    test.assert(saveFileLoad(loaded), true, "Committed record loads");
    test.assert(memcmp(&loaded, &original, sizeof(original)) == 0, true,
                "Commit and load round-trip every SaveFile field");

    flashFakeReset();
    SaveFile preserved = save_test_detail::state(0x5678, 0x6b);
    test.assert(saveFileLoad(preserved), false, "Blank sector has no committed save");
    test.assert(preserved.playerLocation, static_cast<uint16_t>(0x5678),
                "Blank load preserves caller state");

    saveFileCommit(original);
    uint8_t badVersion = static_cast<uint8_t>(SAVE_VERSION + 1);
    flashFakeSetBytes(2, &badVersion, 1);
    test.assert(saveFileLoad(loaded), false, "Wrong version is rejected");

    flashFakeReset();
    saveFileCommit(original);
    uint8_t corrupt = 0;
    flashFakeSetBytes(2 + offsetof(SaveFile, inventory), &corrupt, 1);
    test.assert(saveFileLoad(loaded), false, "Corrupted payload checksum is rejected");

    suite.addTest(test);
}

inline void JournalAppendReplayAndEraseTest(TestSuite &suite)
{
    Test test = Test(__func__);
    flashFakeReset();

    journalAppend(save_test_detail::record(1));
    journalAppend(save_test_detail::record(2));
    journalAppend(save_test_detail::record(3));
    test.assert(journalCount(), static_cast<uint16_t>(3), "Journal counts appended records");
    save_test_detail::replayOrder = 0;
    test.assert(journalReplay(save_test_detail::recordOrder), static_cast<uint16_t>(3),
                "Journal replays every appended record");
    test.assert(save_test_detail::replayOrder, static_cast<uint16_t>(123),
                "Journal replay preserves append order");

    journalErase();
    test.assert(journalCount(), static_cast<uint16_t>(0), "Journal erase returns sector to blank");
    test.assert(flashFakeData()[4096], static_cast<uint8_t>(0xff), "Journal erase fills with 0xff");

    journalAppend(save_test_detail::record(4));
    const uint8_t torn[JOURNAL_RECORD_BYTES] = {5, 0, 0, 1, 2, 3, 4, 0};
    flashFakeSetBytes(4096 + JOURNAL_RECORD_BYTES, torn, sizeof(torn));
    test.assert(journalCount(), static_cast<uint16_t>(1), "Torn tail stops journal scan");
    test.assert(journalReplay(save_test_detail::recordOrder), static_cast<uint16_t>(1),
                "Torn tail is not replayed");

    suite.addTest(test);
}

inline void JournalFullSectorRefusalTest(TestSuite &suite)
{
    Test test = Test(__func__);
    flashFakeReset();

    for (uint16_t i = 0; i < JOURNAL_CAPACITY; ++i) {
        test.assert(journalAppend(save_test_detail::record(static_cast<uint8_t>(i))), true,
                    "Journal accepts each slot through capacity");
    }
    const uint8_t finalByte = flashFakeData()[8191];
    test.assert(journalFull(), true, "Journal reports a full sector");
    test.assert(journalAppend(save_test_detail::record(99)), false,
                "Journal rejects record 513 without erase");
    test.assert(journalCount(), JOURNAL_CAPACITY, "Refusal leaves full journal intact");
    test.assert(flashFakeData()[8191], finalByte, "Refusal leaves flash unchanged");

    suite.addTest(test);
}

inline void CompactionSequenceAndInterruptionsTest(TestSuite &suite)
{
    Test test = Test(__func__);
    flashFakeReset();

    SaveFile previous = save_test_detail::state(0x0102, 0x11);
    saveFileCommit(previous);
    journalAppend(save_test_detail::record(0x44));
    SaveFile next = save_test_detail::state(0x0304, 0x22);
    saveBegin();
    save_test_detail::advanceToDone(next);
    SaveFile loaded = {};
    test.assert(saveFileLoad(loaded), true, "Clean compaction leaves valid save");
    test.assert(loaded.playerLocation, static_cast<uint16_t>(0x0304),
                "Compaction commits live fields");
    test.assert(journalCount(), static_cast<uint16_t>(0), "Compaction erases journal after verify");

    flashFakeReset();
    previous = save_test_detail::state(0x1112, 0x33);
    saveFileCommit(previous);
    journalAppend(save_test_detail::record(0x55));
    next = save_test_detail::state(0x1314, 0x44);
    saveBegin();
    saveStepAdvance(next);
    flashFakeSetWriteLimit(0);
    saveStepAdvance(next);
    flashFakeSetWriteLimit(-1);
    test.assert(saveStepAdvance(next), SaveStep::Failed,
                "Torn commit fails verification");
    test.assert(saveFileLoad(loaded), true, "Torn commit retains previous valid record");
    test.assert(loaded.playerLocation, static_cast<uint16_t>(0x1112),
                "Interrupted commit loads previous state");
    test.assert(journalCount(), static_cast<uint16_t>(1), "Interrupted commit retains journal");

    flashFakeReset();
    journalAppend(save_test_detail::record(0x66));
    next = save_test_detail::state(0x1516, 0x55);
    saveBegin();
    saveStepAdvance(next);
    saveStepAdvance(next);
    test.assert(saveStepAdvance(next), SaveStep::EraseJournal,
                "Verified save advances to journal erase");
    flashFakeSetWriteLimit(0);
    saveStepAdvance(next);
    flashFakeSetWriteLimit(-1);
    test.assert(saveStepAdvance(next), SaveStep::Done,
                "Interrupted erase still completes committed sequence");
    test.assert(saveFileLoad(loaded), true, "Interrupted erase retains new record");
    test.assert(loaded.playerLocation, static_cast<uint16_t>(0x1516),
                "Interrupted erase loads new state");
    save_test_detail::replayState = &loaded;
    test.assert(journalReplay(save_test_detail::applyRecord), static_cast<uint16_t>(1),
                "Stale journal remains replayable after interrupted erase");
    save_test_detail::replayState = nullptr;
    test.assert(reinterpret_cast<uint8_t *>(&loaded.party[0])[0], static_cast<uint8_t>(0x66),
                "Replaying stale slot mutation is idempotent");

    suite.addTest(test);
}

inline void CompactionBusyGateTest(TestSuite &suite)
{
    Test test = Test(__func__);
    flashFakeReset();
    SaveFile state = save_test_detail::state(0x9999, 0xaa);
    saveBegin();
    flashFakeSetBusy(true);
    flashFakeResetReadCount();
    test.assert(saveStepAdvance(state), SaveStep::Replay, "Busy save retains current step");
    test.assert(flashFakeReadCount(), static_cast<uint32_t>(0),
                "Busy save performs no flash read");
    flashFakeSetBusy(false);

    suite.addTest(test);
}

inline void SaveSuite(TestRunner &runner)
{
    TestSuite suite = TestSuite("Save Suite");
    SaveFileRoundTripAndValidationTest(suite);
    JournalAppendReplayAndEraseTest(suite);
    JournalFullSectorRefusalTest(suite);
    CompactionSequenceAndInterruptionsTest(suite);
    CompactionBusyGateTest(suite);
    runner.addTestSuite(suite);
}
