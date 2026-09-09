#pragma once

#include <string.h>

#include "fxtest.hpp"
#include "generated/rawread_data.hpp"
#include "src/fxdata.h"
#include "src/lib/ReadData.hpp"
#include "src/save/Compaction.hpp"
#include "src/save/Journal.hpp"
#include "src/save/SaveFile.hpp"

namespace save_fx_test_detail {
inline uint8_t replayedPayload = 0;

inline JournalRecord record(uint8_t value, uint8_t slot = 0, uint8_t op = 0)
{
    JournalRecord result = {};
    result.seq = 1;
    result.op = op;
    result.slot = slot;
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
    return static_cast<uint16_t>(bytes[0] << 8) | static_cast<uint16_t>(bytes[1]);
}

inline uint8_t saveByte(uint24_t address)
{
    uint8_t byte = 0;
    FX::readSaveBytes(address, &byte, sizeof(byte));
    return byte;
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

    JournalRecord added = save_fx_test_detail::record(0x47, 0, 0);
    JournalRecord removed = save_fx_test_detail::record(0x88, 1, 1);
    test.expectEq(journalAppend(added), true, F("journal add appends in log sector"));
    test.expectEq(journalAppend(removed), true, F("journal remove appends in log sector"));
    test.expectEq(save_fx_test_detail::saveByte(save_log), added.seq,
                  F("journal uses separate log sector"));
    test.expectEq(journalCount(), static_cast<uint16_t>(2), F("journal count sector 1"));
    save_fx_test_detail::replayedPayload = 0;
    test.expectEq(journalReplay(save_fx_test_detail::replay), static_cast<uint16_t>(2),
                  F("journal replays add and remove"));
    test.expectEq(save_fx_test_detail::replayedPayload, static_cast<uint8_t>(0x88),
                  F("journal replay reaches final record"));

    SaveFile compacted = save_fx_test_detail::state(0x9abc, 0x73);
    saveBegin();
    SaveStep step = SaveStep::Idle;
    for (uint8_t attempts = 0; attempts < 32 && saveInProgress(); ++attempts) {
        step = saveStepAdvance(compacted);
        FX::waitWhileBusy();
    }
    test.expectEq(static_cast<uint8_t>(step), static_cast<uint8_t>(SaveStep::Done),
                  F("two-sector compaction reaches Done"));
    test.expectEq(journalCount(), static_cast<uint16_t>(0), F("Done leaves log sector blank"));
    test.expectEq(save_fx_test_detail::saveByte(save_log), static_cast<uint8_t>(0xff),
                  F("Done erases log sector"));
    test.expectEq(saveFileLoad(loaded), true, F("compacted record loads"));
    test.expectEq(loaded.playerLocation, compacted.playerLocation,
                  F("compacted record preserves live fields"));

    const uint8_t *addedParty = reinterpret_cast<const uint8_t *>(&loaded.party[0]);
    for (uint8_t index = 0; index < sizeof(added.payload); ++index) {
        test.expectEq(addedParty[index], added.payload[index],
                      F("journal add transfers creature bytes"));
    }
    for (uint16_t index = sizeof(added.payload); index < sizeof(Creature); ++index) {
        test.expectEq(addedParty[index], static_cast<uint8_t>(0x62),
                      F("journal add preserves committed creature tail"));
    }
    const uint8_t *removedParty = reinterpret_cast<const uint8_t *>(&loaded.party[1]);
    for (uint16_t index = 0; index < sizeof(Creature); ++index) {
        test.expectEq(removedParty[index], static_cast<uint8_t>(0),
                      F("journal remove clears creature slot"));
    }

    FX::waitWhileBusy();
    test.expectEq(ReadFXu16(move_table), rawReadMoveTableFirst,
                  F("FX data read after wait"));
}
