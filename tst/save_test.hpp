#pragma once

#include <string.h>

#include "test.hpp"
#include "../src/save/Compaction.hpp"
#include "../src/save/FlashBackend.hpp"
#include "../src/save/Journal.hpp"

namespace save_test_detail {
inline void advanceToDone(SaveFile &state)
{
    while (saveInProgress()) {
        saveStepAdvance(state);
    }
}

inline void appendAdd(uint8_t slot, const uint8_t payload[4])
{
    JournalRecord record = {};
    record.seq = 1;
    record.op = 0;
    record.slot = slot;
    memcpy(record.payload, payload, sizeof(record.payload));
    journalAppend(record);
}
} // namespace save_test_detail

inline void SaveFileLoadPreservesStateTest(TestSuite &suite)
{
    Test test = Test(__func__);
    flashFakeReset();

    SaveFile live = {};
    memset(&live, 0x6b, sizeof(live));
    test.assert(saveFileLoad(live), false,
                "Missing committed save reports failure");
    test.assert(reinterpret_cast<const uint8_t *>(&live)[0], static_cast<uint8_t>(0x6b),
                "Missing committed save preserves caller state");

    suite.addTest(test);
}

inline void CompactionUsesCommittedPartyBaselineTest(TestSuite &suite)
{
    Test test = Test(__func__);
    flashFakeReset();

    SaveFile committed = {};
    committed.version = SAVE_VERSION;
    memset(committed.party, 0x5a, sizeof(committed.party));
    saveFileCommit(committed);

    const uint8_t payload[4] = {1, 2, 3, 4};
    save_test_detail::appendAdd(0, payload);

    SaveFile live = {};
    live.playerLocation = 0x1234;
    memset(live.party, 0xa5, sizeof(live.party));
    saveBegin();
    save_test_detail::advanceToDone(live);

    SaveFile persisted = {};
    test.assert(saveFileLoad(persisted), true, "Compaction commits replayed save");
    test.assert(persisted.playerLocation, static_cast<uint16_t>(0x1234),
                "Compaction retains caller-owned live fields");
    test.assert(reinterpret_cast<const uint8_t *>(&persisted.party[0])[0], payload[0],
                "Journal add replaces party payload");
    test.assert(reinterpret_cast<const uint8_t *>(&persisted.party[0])[4], static_cast<uint8_t>(0x5a),
                "Replay starts from committed party baseline");
    test.assert(journalCount(), static_cast<uint16_t>(0),
                "Journal erases only after verified commit");

    suite.addTest(test);
}

inline void CompactionKeepsNewGameStateWithoutCommitTest(TestSuite &suite)
{
    Test test = Test(__func__);
    flashFakeReset();

    const uint8_t payload[4] = {7, 8, 9, 10};
    save_test_detail::appendAdd(0, payload);

    SaveFile live = {};
    live.playerLocation = 0x4321;
    memset(live.party, 0x33, sizeof(live.party));
    saveBegin();
    save_test_detail::advanceToDone(live);

    SaveFile persisted = {};
    test.assert(saveFileLoad(persisted), true, "New-game compaction commits state");
    test.assert(persisted.playerLocation, static_cast<uint16_t>(0x4321),
                "No committed record preserves live fields");
    test.assert(reinterpret_cast<const uint8_t *>(&persisted.party[0])[4], static_cast<uint8_t>(0x33),
                "No committed record preserves live party baseline");

    suite.addTest(test);
}

inline void SaveSuite(TestRunner &runner)
{
    TestSuite suite = TestSuite("Save Suite");
    SaveFileLoadPreservesStateTest(suite);
    CompactionUsesCommittedPartyBaselineTest(suite);
    CompactionKeepsNewGameStateWithoutCommitTest(suite);
    runner.addTestSuite(suite);
}
