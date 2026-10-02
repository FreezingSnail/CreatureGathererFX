#pragma once

#include <string.h>

#include "test.hpp"
#include "../src/save/Compaction.hpp"
#include "../src/save/FlashBackend.hpp"
#include "../src/save/Journal.hpp"
#include "../src/save/SaveFile.hpp"
#include "../src/save/StoreRecord.hpp"

namespace save_test_detail {
inline void advanceToDone(SaveFile &state)
{
    for (uint8_t i = 0; i < 16 && saveInProgress(); ++i) {
        saveStepAdvance(state);
    }
}

inline JournalRecord record(uint8_t value, uint16_t slot = 0,
                            LogOp op = LogOp::StoreAdd)
{
    JournalRecord result = {};
    result.seq = value;
    result.op = static_cast<uint8_t>(op);
    result.slot = slot;
    for (uint8_t i = 0; i < sizeof(result.payload); ++i) {
        result.payload[i] = static_cast<uint8_t>(value + i);
    }
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

inline void StoreRecordLayoutAndAddressTest(TestSuite &suite)
{
    Test test = Test(__func__);
    constexpr uint24_t base = 0x19000;
    uint8_t sector[4096];
    memset(sector, 0xff, sizeof(sector));
    const uint16_t slots[] = {0, 257, 511};
    for (uint16_t slot : slots) {
        const uint24_t address = storeRecordAddr(base, slot);
        const uint16_t offset = static_cast<uint16_t>(slot << 3);
        test.assert(address, base + offset,
                    "Store slot address " + std::to_string(slot));
        test.assert(storeRecordAddr(base, slot + 1) - address,
                    static_cast<uint24_t>(STORE_RECORD_BYTES),
                    "Store slot stride " + std::to_string(slot));
        test.assert(sector[offset], STORE_RECORD_TOMBSTONE_ID,
                    "Erased slot is a tombstone " + std::to_string(slot));

        const StoreRecord original = {
            static_cast<uint8_t>(slot + 1), static_cast<uint16_t>(0x1200 + slot),
            {2, 3, 4, 5}, 0};
        memcpy(&sector[offset], &original, sizeof(original));
        StoreRecord decoded = {};
        memcpy(&decoded, &sector[offset], sizeof(decoded));
        test.assert(memcmp(&decoded, &original, sizeof(original)), 0,
                    "Store slot round-trips " + std::to_string(slot));
        test.assert(sector[offset + 0], original.id, "Packed id");
        test.assert(sector[offset + 1], static_cast<uint8_t>(original.exp),
                    "Packed experience low byte");
        test.assert(sector[offset + 2], static_cast<uint8_t>(original.exp >> 8),
                    "Packed experience high byte");
        test.assert(sector[offset + 7], static_cast<uint8_t>(0),
                    "Zero reserved byte is default");
    }
    test.assert(storeRecordAddr(base, 31), base + 248,
                "Last record in first page");
    test.assert(storeRecordAddr(base, 32), base + 256,
                "First record in second page");
    test.assert(storeRecordAddr(base, STORE_RECORDS_PER_SECTOR - 1),
                base + 4096 - STORE_RECORD_BYTES,
                "Last record fits inside sector");
    suite.addTest(test);
}

inline void SaveRecordEncodeDecodeTest(TestSuite &suite)
{
    Test test = Test(__func__);
    flashFakeReset();
    JournalRecord source = save_test_detail::record(7, 0x1234, LogOp::StoreAdd);
    const StoreRecord stored = {7, 0x1234, {2, 3, 4, 5}, 0};
    memcpy(source.payload, &stored, sizeof(stored));
    source.payload[8] = 0x5a;
    source.payload[9] = 0xa5;
    uint8_t encoded[JOURNAL_RECORD_BYTES] = {};
    JournalRecord decoded = {};
    test.assert(journalEncode(source, encoded), true, "Record encode succeeds");
    test.assert(journalDecode(encoded, decoded), true, "Record decode succeeds");
    test.assert(decoded.seq, source.seq, "Sequence round-trips");
    test.assert(decoded.op, source.op, "Operation round-trips");
    test.assert(decoded.slot, source.slot, "16-bit slot round-trips");
    test.assert(memcmp(decoded.payload, source.payload, sizeof(source.payload)), 0,
                "Full store payload round-trips");
    test.assert(encoded[2], static_cast<uint8_t>(0x34), "Slot low byte");
    test.assert(encoded[3], static_cast<uint8_t>(0x12), "Slot high byte");
    test.assert(encoded[15], static_cast<uint8_t>(0xff), "Pad stays erased");
    uint8_t checked = 0;
    for (uint8_t i = 0; i <= 14; ++i) {
        checked ^= encoded[i];
    }
    test.assert(checked, static_cast<uint8_t>(0xff),
                "Check covers bytes 0 through 14");
    uint8_t blank[JOURNAL_RECORD_BYTES];
    memset(blank, 0xff, sizeof(blank));
    test.assert(journalDecode(blank, decoded), false, "All-FF record is blank");
    encoded[13] ^= 1;
    test.assert(journalDecode(encoded, decoded), false,
                "Flipped payload fails record check");
    suite.addTest(test);
}

inline void JournalUnknownOpAndBootTailTest(TestSuite &suite)
{
    Test test = Test(__func__);
    flashFakeReset();
    journalInit();
    JournalRecord legacy = save_test_detail::record(1);
    legacy.op = 0;
    JournalRecord future = save_test_detail::record(2);
    future.op = 99;
    test.assert(journalAppend(legacy), true, "Legacy opcode is stored");
    test.assert(journalAppend(future), true, "Future opcode is stored");
    test.assert(journalAppend(save_test_detail::record(3)), true,
                "Known opcode follows unknown records");
    save_test_detail::replayOrder = 0;
    test.assert(journalReplay(save_test_detail::recordOrder), static_cast<uint16_t>(3),
                "Replay counts all valid records");
    test.assert(save_test_detail::replayOrder, static_cast<uint16_t>(3),
                "Replay skips unknown operations and continues");
    journalInit(); // Simulated boot scan of a nonempty sector.
    flashFakeResetReadCount();
    test.assert(journalAppend(save_test_detail::record(4)), true,
                "Boot-scanned tail accepts next record");
    test.assert(flashFakeReadCount() <= static_cast<uint32_t>(1), true,
                "Append after boot scan uses at most one flash read");
    test.assert(journalCount(), static_cast<uint16_t>(4),
                "Boot-scanned append does not overwrite prior records");
    suite.addTest(test);
}

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

inline void SaveFileLegacyV1DiscardMigrationTest(TestSuite &suite)
{
    Test test = Test(__func__);
    constexpr uint16_t legacyBytes = 157;
    const uint8_t legacyHeader[2] = {
        static_cast<uint8_t>(legacyBytes >> 8), static_cast<uint8_t>(legacyBytes)};
    uint8_t legacyPayload[legacyBytes] = {SAVE_VERSION};

    flashFakeReset();
    flashFakeSetBytes(0, legacyHeader, sizeof(legacyHeader));
    flashFakeSetBytes(sizeof(legacyHeader), legacyPayload, sizeof(legacyPayload));
    SaveFile preserved = save_test_detail::state(0x4567, 0x3c);
    const SaveFile before = preserved;
    test.assert(saveFileLoad(preserved), false,
                "Legacy AVR v1 save is discarded");
    test.assert(memcmp(&preserved, &before, sizeof(preserved)), 0,
                "Legacy-only load leaves caller state unchanged");

    SaveFile current = save_test_detail::state(0x89ab, 0x6d);
    current.version = SAVE_VERSION;
    current.checksum = saveFileChecksum(current);
    const uint16_t currentOffset = static_cast<uint16_t>(2 + legacyBytes);
    const uint8_t currentHeader[2] = {
        static_cast<uint8_t>(sizeof(SaveFile) >> 8),
        static_cast<uint8_t>(sizeof(SaveFile))};
    flashFakeSetBytes(currentOffset, currentHeader, sizeof(currentHeader));
    flashFakeSetBytes(currentOffset + sizeof(currentHeader),
                      reinterpret_cast<const uint8_t *>(&current), sizeof(current));
    SaveFile loaded = {};
    test.assert(saveFileLoad(loaded), true,
                "Current-format save after legacy v1 data loads");
    test.assert(memcmp(&loaded, &current, sizeof(current)), 0,
                "Current-format record after legacy data round-trips");

    flashFakeReset();
    flashFakeSetBytes(0, legacyHeader, sizeof(legacyHeader));
    flashFakeSetBytes(sizeof(legacyHeader), legacyPayload, sizeof(legacyPayload));
    saveFileCommit(current);
    test.assert(flashFakeData()[0], static_cast<uint8_t>(sizeof(SaveFile) >> 8),
                "First current save erases the legacy sector");
    loaded = {};
    test.assert(saveFileLoad(loaded), true,
                "Current save loads after legacy sector discard");

    suite.addTest(test);
}

inline void SaveFileStreamingSelectionTest(TestSuite &suite)
{
    Test test = Test(__func__);
    flashFakeReset();

    SaveFile first = save_test_detail::state(0x1111, 0x11);
    SaveFile second = save_test_detail::state(0x2222, 0x22);
    saveFileCommit(first);
    saveFileCommit(second);
    first.version = second.version = SAVE_VERSION;
    first.checksum = saveFileChecksum(first);
    second.checksum = saveFileChecksum(second);

    SaveFile live = save_test_detail::state(0x7777, 0x77);
    const SaveFile original = live;
    test.assert(saveFileLoadParty(live), true,
                "Party loader selects last valid record");
    test.assert(memcmp(live.party, second.party, sizeof(live.party)), 0,
                "Last valid party is loaded");
    test.assert(memcmp(&live, &original, offsetof(SaveFile, party)), 0,
                "Party load preserves fields before party");
    constexpr size_t afterParty = offsetof(SaveFile, plants);
    test.assert(memcmp(reinterpret_cast<const uint8_t *>(&live) + afterParty,
                       reinterpret_cast<const uint8_t *>(&original) + afterParty,
                       sizeof(SaveFile) - afterParty), 0,
                "Party load preserves plants and inventory");
    test.assert(saveFileMatchesStored(second), true,
                "Streaming verify matches latest record");
    test.assert(saveFileMatchesStored(first), false,
                "Streaming verify rejects an older record");

    const uint16_t secondOffset = static_cast<uint16_t>(sizeof(SaveFile) + 4);
    const uint8_t corrupt = static_cast<uint8_t>(
        flashFakeData()[secondOffset + offsetof(SaveFile, inventory)] ^ 1);
    flashFakeSetBytes(secondOffset + offsetof(SaveFile, inventory), &corrupt, 1);
    test.assert(saveFileLoadParty(live), true,
                "Invalid tail falls back to previous valid record");
    test.assert(memcmp(live.party, first.party, sizeof(live.party)), 0,
                "Fallback party is first valid record");
    test.assert(saveFileMatchesStored(first), true,
                "Streaming verify uses fallback record");

    flashFakeReset();
    const SaveFile beforeFailure = live;
    test.assert(saveFileLoadParty(live), false,
                "Blank sector has no party baseline");
    test.assert(memcmp(&live, &beforeFailure, sizeof(live)), 0,
                "Failed party load preserves caller state byte for byte");
    suite.addTest(test);
}

inline void JournalAppendReplayAndEraseTest(TestSuite &suite)
{
    Test test = Test(__func__);
    flashFakeReset();
    journalInit();

    flashFakeResetReadCount();
    journalAppend(save_test_detail::record(1));
    test.assert(flashFakeReadCount() <= static_cast<uint32_t>(1), true,
                "Post-init append reads at most one record window");
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
    uint8_t torn[JOURNAL_RECORD_BYTES] = {};
    torn[0] = 5;
    flashFakeSetBytes(4096 + JOURNAL_RECORD_BYTES, torn, sizeof(torn));
    test.assert(journalCount(), static_cast<uint16_t>(1), "Torn tail stops journal scan");
    test.assert(journalReplay(save_test_detail::recordOrder), static_cast<uint16_t>(1),
                "Torn tail is not replayed");
    journalInit();
    test.assert(journalAppend(save_test_detail::record(6)), false,
                "Torn tail cannot be overwritten");

    suite.addTest(test);
}

inline void JournalFullSectorRefusalTest(TestSuite &suite)
{
    Test test = Test(__func__);
    flashFakeReset();
    journalInit();

    for (uint16_t i = 0; i < JOURNAL_CAPACITY; ++i) {
        test.assert(journalAppend(save_test_detail::record(static_cast<uint8_t>(i))), true,
                    "Journal accepts each slot through capacity");
    }
    static uint8_t before[JOURNAL_RECORD_BYTES * JOURNAL_CAPACITY];
    memcpy(before, flashFakeData() + 4096, sizeof(before));
    test.assert(journalFull(), true, "Journal reports a full sector");
    test.assert(journalAppend(save_test_detail::record(99)), false,
                "Journal rejects record 257 without erase");
    test.assert(memcmp(flashFakeData() + 4096, before, sizeof(before)), 0,
                "Refusal leaves the entire journal sector unchanged");

    suite.addTest(test);
}

inline void CompactionSequenceAndInterruptionsTest(TestSuite &suite)
{
    Test test = Test(__func__);
    flashFakeReset();
    journalInit();

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
    test.assert(memcmp(loaded.party, next.party, sizeof(next.party)), 0,
                "Store operation cannot modify live party snapshot");
    test.assert(journalCount(), static_cast<uint16_t>(0), "Compaction erases journal after verify");

    flashFakeReset();
    journalInit();
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
    journalInit();
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

inline void VerifyMismatchPreservesJournalTest(TestSuite &suite)
{
    Test test = Test(__func__);
    flashFakeReset();
    journalInit();
    journalAppend(save_test_detail::record(0x77));
    SaveFile state = save_test_detail::state(0x2021, 0x44);
    saveBegin();
    saveStepAdvance(state);
    saveStepAdvance(state);
    uint8_t corrupt = static_cast<uint8_t>(
        flashFakeData()[2 + offsetof(SaveFile, inventory)] ^ 1);
    flashFakeSetBytes(2 + offsetof(SaveFile, inventory), &corrupt, 1);
    test.assert(saveStepAdvance(state), SaveStep::Failed,
                "Verify mismatch ends in Failed");
    test.assert(journalCount(), static_cast<uint16_t>(1),
                "Verify mismatch preserves journal");
    test.assert(flashFakeData()[4096] == 0xff, false,
                "Verify mismatch does not erase journal start");
    suite.addTest(test);
}

inline void CompactionBusyGateTest(TestSuite &suite)
{
    Test test = Test(__func__);
    flashFakeReset();
    journalInit();
    journalAppend(save_test_detail::record(1));
    SaveFile state = save_test_detail::state(0x9999, 0xaa);
    saveBegin();

    flashFakeSetBusy(true);
    flashFakeResetReadCount();
    test.assert(saveStepAdvance(state), SaveStep::Replay,
                "Busy Replay retains current step");
    test.assert(flashFakeReadCount(), static_cast<uint32_t>(0),
                "Busy Replay performs no flash read");
    flashFakeSetBusy(false);
    test.assert(saveStepAdvance(state), SaveStep::Commit, "Replay advances");

    flashFakeSetBusy(true);
    flashFakeResetReadCount();
    test.assert(saveStepAdvance(state), SaveStep::Commit,
                "Busy Commit retains current step");
    test.assert(flashFakeReadCount(), static_cast<uint32_t>(0),
                "Busy Commit performs no flash read");
    flashFakeSetBusy(false);
    test.assert(saveStepAdvance(state), SaveStep::Verify, "Commit advances");

    flashFakeSetBusy(true);
    flashFakeResetReadCount();
    test.assert(saveStepAdvance(state), SaveStep::Verify,
                "Busy Verify retains current step");
    test.assert(flashFakeReadCount(), static_cast<uint32_t>(0),
                "Busy Verify performs no flash read");
    flashFakeSetBusy(false);
    test.assert(saveStepAdvance(state), SaveStep::EraseJournal,
                "Verify advances");

    flashFakeSetBusy(true);
    flashFakeResetReadCount();
    test.assert(saveStepAdvance(state), SaveStep::EraseJournal,
                "Busy EraseJournal retains current step");
    test.assert(flashFakeReadCount(), static_cast<uint32_t>(0),
                "Busy EraseJournal performs no flash read");
    flashFakeSetBusy(false);
    test.assert(saveStepAdvance(state), SaveStep::EraseJournal,
                "EraseJournal issues erase before completion");
    test.assert(saveStepAdvance(state), SaveStep::Done,
                "EraseJournal completes after erase");

    suite.addTest(test);
}

inline void SaveSuite(TestRunner &runner)
{
    TestSuite suite = TestSuite("Save Suite");
    StoreRecordLayoutAndAddressTest(suite);
    SaveRecordEncodeDecodeTest(suite);
    JournalUnknownOpAndBootTailTest(suite);
    SaveFileRoundTripAndValidationTest(suite);
    SaveFileLegacyV1DiscardMigrationTest(suite);
    SaveFileStreamingSelectionTest(suite);
    JournalAppendReplayAndEraseTest(suite);
    JournalFullSectorRefusalTest(suite);
    CompactionSequenceAndInterruptionsTest(suite);
    VerifyMismatchPreservesJournalTest(suite);
    CompactionBusyGateTest(suite);
    runner.addTestSuite(suite);
}
