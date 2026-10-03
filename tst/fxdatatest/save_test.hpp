#pragma once

#include <string.h>

#include "fxtest.hpp"
#include "generated/rawread_data.hpp"
#include "src/fxdata.h"
#include "src/lib/ReadData.hpp"
#include "src/save/Compaction.hpp"
#include "src/save/FlashBackend.hpp"
#include "src/save/Journal.hpp"
#include "src/save/SaveController.hpp"
#include "src/save/SaveFile.hpp"

namespace save_fx_test_detail {
inline uint8_t replayedPayload = 0;

inline JournalRecord record(uint8_t value, uint16_t slot = 0,
                            LogOp op = LogOp::StoreAdd)
{
    JournalRecord result = {};
    result.seq = 1;
    result.op = static_cast<uint8_t>(op);
    result.slot = slot;
    for (uint8_t i = 0; i < sizeof(result.payload); ++i) {
        result.payload[i] = static_cast<uint8_t>(value + i);
    }
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
    memset(result.partyHP, fill, sizeof(result.partyHP));
    memset(&result.plants, fill, sizeof(result.plants));
    memset(result.inventory, fill, sizeof(result.inventory));
    return result;
}

inline void expectPixel(FxTest &test, int16_t x, int16_t y, uint8_t expected,
                        const __FlashStringHelper *label)
{
    const uint8_t *buffer = arduboy.getBuffer();
    const uint16_t index = static_cast<uint16_t>(x + (y / 8) * WIDTH);
    const uint8_t mask = static_cast<uint8_t>(1 << (y % 8));
    test.expectEq((buffer[index] & mask) != 0, expected, label);
}
} // namespace save_fx_test_detail

inline void test_save(FxTest &test)
{
    journalInit();
    SaveFile blank = {};
    test.expectEq(saveFileLoad(blank), false, F("fresh save sector is blank"));

    SaveFile first = save_fx_test_detail::state(0x1234, 0x31);
    first.partyHP[0] = 17;
    first.partyHP[1] = 23;
    first.partyHP[2] = 29;
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
    test.expectEq(loaded.partyHP[0], static_cast<uint8_t>(17), F("first HP survives reload"));
    test.expectEq(loaded.partyHP[1], static_cast<uint8_t>(23), F("second HP survives reload"));
    test.expectEq(loaded.partyHP[2], static_cast<uint8_t>(29), F("third HP survives reload"));

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

    JournalRecord added = save_fx_test_detail::record(0x47, 0x0102, LogOp::StoreAdd);
    JournalRecord removed = save_fx_test_detail::record(0x88, 1, LogOp::StoreRemove);
    test.expectEq(journalAppend(added), true, F("journal add appends in log sector"));
    test.expectEq(journalAppend(removed), true, F("journal remove appends in log sector"));
    test.expectEq(save_fx_test_detail::saveByte(save_log), added.seq,
                  F("journal uses separate log sector"));
    test.expectEq(save_fx_test_detail::saveByte(save_log + 2), static_cast<uint8_t>(2),
                  F("journal slot low byte"));
    test.expectEq(save_fx_test_detail::saveByte(save_log + 3), static_cast<uint8_t>(1),
                  F("journal slot high byte"));
    for (uint8_t i = 0; i < sizeof(added.payload); ++i) {
        test.expectEq(save_fx_test_detail::saveByte(save_log + 4 + i), added.payload[i],
                      F("journal stores full payload"));
    }
    test.expectEq(save_fx_test_detail::saveByte(save_log + JOURNAL_RECORD_BYTES), removed.seq,
                  F("journal second record has 16-byte stride"));
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

    for (uint16_t index = 0; index < sizeof(compacted.party); ++index) {
        test.expectEq(reinterpret_cast<const uint8_t *>(loaded.party)[index],
                      reinterpret_cast<const uint8_t *>(compacted.party)[index],
                      F("store operations preserve live party snapshot"));
    }

    FX::waitWhileBusy();
    test.expectEq(ReadFXu16(move_table), rawReadMoveTableFirst,
                  F("FX data read after wait"));

    // Once the previous save has completed, drawStatus chooses FAILED. Check
    // framebuffer pixels to cover PROGMEM reads in the inactive-save branch.
    SaveController::drawStatus();
    save_fx_test_detail::expectPixel(test, 34, 27, 0, F("FAILED F glyph pixel"));
    save_fx_test_detail::expectPixel(test, 32, 27, 1, F("FAILED screen background"));

    // Start a real asynchronous save-flash erase, then draw while the external
    // chip is busy. The status renderer must use only the internal flash glyphs.
    FX::eraseSaveBlock(0);
    const bool busyBeforeDraw = flash.busy();
    SaveController::begin(GameState_t::WORLD);
    test.expectEq(busyBeforeDraw, true, F("save flash busy before status draw"));
    test.expectEq(saveInProgress(), true, F("save remains active before status draw"));
    SaveController::drawStatus();
    const bool busyAfterDraw = flash.busy();
    test.expectEq(busyAfterDraw, true, F("save flash remains busy after status draw"));
    test.expectEq(saveInProgress(), true, F("save remains active after status draw"));

    // Sample both ink and paper at each letter's first row. This checks the
    // flattened six-by-five glyph selection, not just that the screen changed.
    save_fx_test_detail::expectPixel(test, 34, 27, 1, F("S first pixel paper"));
    save_fx_test_detail::expectPixel(test, 36, 27, 0, F("S second pixel ink"));
    save_fx_test_detail::expectPixel(test, 44, 27, 1, F("A first pixel paper"));
    save_fx_test_detail::expectPixel(test, 46, 27, 0, F("A second pixel ink"));
    save_fx_test_detail::expectPixel(test, 54, 27, 0, F("V first pixel ink"));
    save_fx_test_detail::expectPixel(test, 56, 27, 1, F("V second pixel paper"));
    save_fx_test_detail::expectPixel(test, 64, 27, 0, F("I first pixel ink"));
    save_fx_test_detail::expectPixel(test, 66, 27, 0, F("I second pixel ink"));
    save_fx_test_detail::expectPixel(test, 74, 27, 0, F("N first pixel ink"));
    save_fx_test_detail::expectPixel(test, 76, 27, 1, F("N second pixel paper"));
    save_fx_test_detail::expectPixel(test, 84, 27, 1, F("G first pixel paper"));
    save_fx_test_detail::expectPixel(test, 86, 27, 0, F("G second pixel ink"));
    FX::waitWhileBusy();
}
