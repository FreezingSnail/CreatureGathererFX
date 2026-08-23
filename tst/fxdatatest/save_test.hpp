#pragma once

#include <string.h>

#include "fxtest.hpp"
#include "generated/rawread_data.hpp"
#include "src/fxdata.h"
#include "src/save/Compaction.hpp"
#include "src/save/Journal.hpp"
#include "src/save/SaveFile.hpp"

namespace save_fx_test_detail {
inline uint8_t replayedPayload = 0;

inline JournalRecord record(uint8_t value)
{
    JournalRecord result = {};
    result.seq = 1;
    result.op = 0;
    result.slot = 0;
    result.payload[0] = value;
    result.payload[1] = static_cast<uint8_t>(value + 1);
    result.payload[2] = static_cast<uint8_t>(value + 2);
    result.payload[3] = static_cast<uint8_t>(value + 3);
    return result;
}

inline void replay(const JournalRecord &record)
{
    replayedPayload = record.payload[0];
}

inline uint16_t saveWord(uint16_t address)
{
    uint8_t bytes[2];
    FX::readSaveBytes(address, bytes, sizeof(bytes));
    return static_cast<uint16_t>(bytes[0]) | static_cast<uint16_t>(bytes[1] << 8);
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
} // namespace save_fx_test_detail

inline void test_save(FxTest &test)
{
    SaveFile blank = {};
    test.expectEq(saveFileLoad(blank), false, F("fresh save sector is blank"));

    SaveFile first = save_fx_test_detail::state(0x1234, 0x31);
    saveFileCommit(first);
    first.version = SAVE_VERSION;
    first.checksum = saveFileChecksum(first);
    SaveFile loaded = {};
    test.expectEq(saveFileLoad(loaded), true, F("commit loads from sector 0"));
    for (uint16_t index = 0; index < sizeof(SaveFile); ++index) {
        const uint8_t actual = reinterpret_cast<const uint8_t *>(&loaded)[index];
        const uint8_t expected = reinterpret_cast<const uint8_t *>(&first)[index];
        test.expectEq(actual, expected, F("save round trip byte"));
    }

    SaveFile second = save_fx_test_detail::state(0x5678, 0x62);
    saveFileCommit(second);
    second.version = SAVE_VERSION;
    second.checksum = saveFileChecksum(second);
    test.expectEq(save_fx_test_detail::saveWord(0), static_cast<uint16_t>(sizeof(SaveFile)),
                  F("first record header"));
    test.expectEq(save_fx_test_detail::saveWord(sizeof(SaveFile) + 2),
                  static_cast<uint16_t>(sizeof(SaveFile)),
                  F("repeated commit appends record"));
    test.expectEq(saveFileLoad(loaded), true, F("latest appended record loads"));
    test.expectEq(loaded.playerLocation, second.playerLocation, F("latest record selected"));

    JournalRecord record = save_fx_test_detail::record(0x47);
    test.expectEq(journalAppend(record), true, F("journal append sector 1"));
    test.expectEq(journalCount(), static_cast<uint16_t>(1), F("journal count sector 1"));
    save_fx_test_detail::replayedPayload = 0;
    test.expectEq(journalReplay(save_fx_test_detail::replay), static_cast<uint16_t>(1),
                  F("journal replay count"));
    test.expectEq(save_fx_test_detail::replayedPayload, static_cast<uint8_t>(0x47),
                  F("journal replay payload"));

    SaveFile compacted = save_fx_test_detail::state(0x9abc, 0x73);
    saveBegin();
    SaveStep step = SaveStep::Idle;
    for (uint8_t attempts = 0; attempts < 32 && saveInProgress(); ++attempts) {
        step = saveStepAdvance(compacted);
    }
    test.expectEq(static_cast<uint8_t>(step), static_cast<uint8_t>(SaveStep::Done),
                  F("commit sequence reaches Done"));
    test.expectEq(journalCount(), static_cast<uint16_t>(0), F("Done leaves sector 1 blank"));
    test.expectEq(saveFileLoad(loaded), true, F("compacted record loads"));
    test.expectEq(loaded.playerLocation, compacted.playerLocation,
                  F("compacted record preserved"));

    FX::waitWhileBusy();
    test.expectEq(FX::readIndexedUInt16(move_table, 0), rawReadMoveTableFirst,
                  F("FX data read after wait"));
}
