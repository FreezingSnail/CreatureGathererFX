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
#include "src/save/StoreRecord.hpp"
#include "src/creature/Creature.hpp"
#include "src/creature/LevelCurve.hpp"

// Device mirror of the host v3 save suite. It proves the 96-byte snapshot plus
// 64-slot store contract (docs/m1-storage.md) against the real FX save flash
// backend instead of the host fake. The write limiter below swaps counting
// wrappers into the mutable backend so the same interruption offsets that the
// host suite cuts can also be exercised here.
namespace save_fx_test_detail {

int32_t writeLimit = -1;
void (*baseErase)(uint16_t) = nullptr;
void (*baseWrite)(uint32_t, const uint8_t *, uint16_t) = nullptr;

void limitedErase(uint16_t page)
{
    if (writeLimit == 0) return;
    baseErase(page);
    if (writeLimit > 0) writeLimit -= writeLimit >= 4096 ? 4096 : writeLimit;
}

void limitedWrite(uint32_t addr, const uint8_t *data, uint16_t len)
{
    if (writeLimit == 0) return;
    uint16_t count = len;
    if (writeLimit > 0 && static_cast<uint32_t>(writeLimit) < count)
        count = static_cast<uint16_t>(writeLimit);
    baseWrite(addr, data, count);
    if (writeLimit > 0) writeLimit -= count;
}

void cutBegin(int32_t limit)
{
    baseErase = flash.eraseSector;
    baseWrite = flash.writeBytes;
    flash.eraseSector = limitedErase;
    flash.writeBytes = limitedWrite;
    writeLimit = limit;
}

void cutEnd()
{
    flash.eraseSector = baseErase;
    flash.writeBytes = baseWrite;
    writeLimit = -1;
}

// Clear both v3 checkpoint sectors, then reset the store runtime. The journal
// sector is erased by storeFresh; the packed FX data image is never touched.
void eraseSaves()
{
    // Wait after each erase: the FX flash ignores a command issued while the
    // previous erase is still in flight.
    const uint16_t pages[] = {32, 48}; // save_store_a 0x2000, save_store_b 0x3000
    for (uint8_t i = 0; i < sizeof(pages) / sizeof(pages[0]); ++i) {
        flash.eraseSector(pages[i]);
        while (flash.busy()) {}
    }
}

void fresh() { eraseSaves(); storeFresh(); }

SaveFile snapshot(uint16_t location = 0)
{
    SaveFile state = {};
    state.playerLocation = location;
    memset(state.partySlots, 255, 3);
    return state;
}

StoreRecord creature(uint16_t exp = 27)
{
    StoreRecord record = {};
    record.id = 0;
    record.exp = exp;
    record.moves[0] = 8;
    record.moves[1] = 32;
    record.moves[2] = 32;
    record.moves[3] = 32;
    return record;
}

SaveStep save(SaveFile &state)
{
    saveBegin();
    SaveStep result = SaveStep::Idle;
    for (uint16_t i = 0; i < 256 && saveInProgress(); ++i) {
        result = saveStepAdvance(state);
        FX::waitWhileBusy();
    }
    return result;
}

uint8_t saveByte(uint24_t address)
{
    uint8_t byte = 0;
    flash.readBytes(address, &byte, 1);
    return byte;
}

JournalRecord journalRecord(uint8_t value, uint16_t slot = 0,
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

uint8_t replayedPayload = 0;
void replay(const JournalRecord &record) { replayedPayload = record.payload[0]; }

// NOR-safe in-place corruption: clears the masked bits of one byte. Both AND
// and overwrite flash write models produce the same written value.
void clearByteBits(uint24_t address, uint8_t mask)
{
    uint8_t byte = 0;
    flash.readBytes(address, &byte, 1);
    byte = static_cast<uint8_t>(byte & mask);
    flash.writeBytes(address, &byte, 1);
}

void expectPixel(FxTest &test, int16_t x, int16_t y, uint8_t expected,
                 const __FlashStringHelper *label)
{
    const uint8_t *buffer = arduboy.getBuffer();
    const uint16_t index = static_cast<uint16_t>(x + (y / 8) * WIDTH);
    const uint8_t mask = static_cast<uint8_t>(1 << (y % 8));
    test.expectEq((buffer[index] & mask) != 0, expected, label);
}
} // namespace save_fx_test_detail

// The on-flash 16-byte journal record format and separate log sector are part
// of the v3 contract even though the snapshot no longer carries a checksum.
inline void test_saveJournalLayout(FxTest &test)
{
    using namespace save_fx_test_detail;
    journalErase();
    while (flash.busy()) {}
    JournalRecord added = journalRecord(0x47, 0x0102, LogOp::StoreAdd);
    JournalRecord removed = journalRecord(0x88, 1, LogOp::StoreRemove);
    test.expectEq(journalAppend(added), true, F("journal add appends in log sector"));
    test.expectEq(journalAppend(removed), true, F("journal remove appends in log sector"));
    test.expectEq(saveByte(save_log), added.seq, F("journal stores sequence first"));
    test.expectEq(saveByte(save_log + 1), static_cast<uint8_t>(LogOp::StoreAdd),
                  F("journal stores op"));
    test.expectEq(saveByte(save_log + 2), static_cast<uint8_t>(2), F("journal slot low byte"));
    test.expectEq(saveByte(save_log + 3), static_cast<uint8_t>(1), F("journal slot high byte"));
    for (uint8_t i = 0; i < sizeof(added.payload); ++i) {
        test.expectEq(saveByte(save_log + 4 + i), added.payload[i],
                      F("journal stores full payload"));
    }
    test.expectEq(saveByte(save_log + JOURNAL_RECORD_BYTES), removed.seq,
                  F("journal second record has 16-byte stride"));
    test.expectEq(saveByte(save_log + JOURNAL_RECORD_BYTES + 15), static_cast<uint8_t>(0xa5),
                  F("journal publish marker"));
    test.expectEq(journalCount(), static_cast<uint16_t>(2), F("journal count sector 1"));
    replayedPayload = 0;
    test.expectEq(journalReplay(replay), static_cast<uint16_t>(2),
                  F("journal replays add and remove"));
    test.expectEq(replayedPayload, static_cast<uint8_t>(0x88),
                  F("journal replay reaches final record"));
}

inline void test_saveCheckpointCollection(FxTest &test)
{
    using namespace save_fx_test_detail;
    fresh();
    SaveFile state = snapshot(0x1234);
    StoreRecord record = creature();
    uint8_t slot = 255;
    test.expectEq(storeAdd(record, slot), true, F("first instance add"));
    test.expectEq(slot, 0, F("first stable slot"));
    test.expectEq(storeAssign(0, slot, 17), true, F("party references instance"));
    record.exp = 125;
    test.expectEq(storeAdd(record, slot), true, F("duplicate species add"));
    test.expectEq(slot, 1, F("duplicate owns separate slot"));
    test.expectEq(storeRead(63, record), true, F("last empty slot readable"));
    test.expectEq(record.id, 255, F("empty is canonical tombstone"));
    test.expectEq(storeRead(64, record), false, F("out of range refused"));
    test.expectEq(static_cast<uint8_t>(save(state)), static_cast<uint8_t>(SaveStep::Done),
                  F("coherent save completes"));
    test.expectEq(saveByte(save_store_a + 9), static_cast<uint8_t>(0),
                  F("cleanup bit clear after journal verification"));
    SaveFile loaded = snapshot();
    test.expectEq(storeBoot(loaded), true, F("power cycle selects checkpoint"));
    test.expectEq(loaded.playerLocation, 0x1234, F("snapshot location shares checkpoint"));
    test.expectEq(loaded.partySlots[0], 0, F("party stable reference restored"));
    test.expectEq(loaded.partyHP[0], 17, F("assignment HP restored explicitly"));
    test.expectEq(storeRead(1, record), true, F("duplicate restored"));
    test.expectEq(record.exp, 125, F("duplicate experience retained"));
    Creature instance;
    instance.load(record);
    test.expectEq(instance.level, levelFromExp(125), F("derived level restored"));
    test.expectEq(journalCount(), static_cast<uint16_t>(0),
                  F("journal erased after coherent publication"));
    test.expectEq(storeAssign(0, 1, 11), true, F("new occupant carries explicit HP"));
    test.expectEq(storePartyHP(0), 11, F("old occupant HP is replaced"));
    test.expectEq(storeAssign(1, 1, 11), false, F("duplicate party reference refused"));
    test.expectEq(storeAssign(0, 255, 1), false, F("unassignment requires zero HP"));
    test.expectEq(storeAssign(0, 1, 255), false, F("HP above derived maximum refused"));
    test.expectEq(storeBoot(loaded), true, F("assignment journal replays on reboot"));
    test.expectEq(loaded.partySlots[0], 1, F("new party occupant restored"));
    test.expectEq(loaded.partyHP[0], 11, F("journal restores new occupant HP"));
    test.expectEq(storeRemove(1), false, F("party slot cannot be removed"));
    test.expectEq(storeRecordAddr(save_store_a, 63), save_store_a + 616,
                  F("slot63 starts at624 minus8"));
}

inline void test_saveCollectionCapacity(FxTest &test)
{
    using namespace save_fx_test_detail;
    fresh();
    SaveFile state = snapshot();
    StoreRecord record = creature();
    uint8_t slot = 255;
    for (uint8_t i = 0; i < 64; ++i) {
        test.expectEq(storeAdd(record, slot), true, F("bounded instance add"));
        test.expectEq(slot, i, F("stable allocation"));
        if (i % 16 == 15) {
            test.expectEq(static_cast<uint8_t>(save(state)),
                          static_cast<uint8_t>(SaveStep::Done), F("explicit batch save"));
        }
    }
    test.expectEq(storeAdd(record, slot), false, F("full64 refuses overwrite"));
    test.expectEq(storeBoot(state), true, F("full checkpoint loads"));
    test.expectEq(storeRead(63, record), true, F("last slot restores"));
    test.expectEq(record.id, 0, F("slot63 occupied"));
    test.expectEq(storeRemove(63), true, F("unreferenced last slot tombstone"));
    test.expectEq(storeAdd(record, slot), true, F("tombstone reused"));
    test.expectEq(slot, 63, F("no slot squeeze"));
}

inline void test_saveCheckpointCuts(FxTest &test)
{
    using namespace save_fx_test_detail;
    const int32_t cuts[] = {4718, 4720, 8816, 8817};
    for (uint8_t c = 0; c < sizeof(cuts) / sizeof(cuts[0]); ++c) {
        fresh();
        SaveFile state = snapshot(11);
        test.expectEq(static_cast<uint8_t>(save(state)),
                      static_cast<uint8_t>(SaveStep::Done), F("baseline checkpoint"));
        StoreRecord record = creature();
        uint8_t slot = 255;
        test.expectEq(storeAdd(record, slot), true, F("postcheckpoint mutation"));
        state.playerLocation = 22;
        cutBegin(cuts[c]);
        save(state);
        cutEnd();
        SaveFile loaded = snapshot();
        test.expectEq(storeBoot(loaded), true, F("cut reboots into valid state"));
        test.expectEq(loaded.playerLocation, cuts[c] >= 4720 ? 22 : 11,
                      F("commit markers define snapshot authority"));
        test.expectEq(storeRead(slot, record), true, F("cut instance read"));
        test.expectEq(record.exp, 27, F("checkpoint or valid journal retains add"));
        test.expectEq(storeBootFailed(), false, F("cut cleanup completed"));
    }
    fresh();
    SaveFile state = snapshot();
    cutBegin(4608);
    save(state);
    cutEnd();
    test.expectEq(storeBoot(state), false, F("interrupted first save resets safely"));
    test.expectEq(storeBootFailed(), false, F("no committed save existed"));
    test.expectEq(static_cast<uint8_t>(save(state)),
                  static_cast<uint8_t>(SaveStep::Done), F("first save retried after reset"));
    // Clear bit 0 of the magic byte: the checkpoint keeps both commit markers
    // but fails integrity, so boot must fail closed.
    clearByteBits(save_store_a, static_cast<uint8_t>(0xfe));
    test.expectEq(storeBoot(state), false, F("invalid committed header rejected"));
    test.expectEq(storeBootFailed(), true, F("committed corruption fails closed"));
}

inline void test_saveJournalPublication(FxTest &test)
{
    using namespace save_fx_test_detail;
    fresh();
    StoreRecord record = creature();
    cutBegin(15);
    test.expectEq(storeWrite(0, record), false, F("body without marker is unpublished"));
    cutEnd();
    test.expectEq(journalCount(), static_cast<uint16_t>(0), F("torn marker stops replay"));
    test.expectEq(storeWrite(0, record), false, F("dirty tail not overwritten"));
    SaveFile state = snapshot();
    test.expectEq(static_cast<uint8_t>(save(state)),
                  static_cast<uint8_t>(SaveStep::Done), F("explicit save clears torn tail"));
    for (uint8_t i = 0; i < 16; ++i) {
        test.expectEq(storeWrite(i, record), true, F("pending index bounded write"));
    }
    const uint16_t count = journalCount();
    test.expectEq(storeWrite(16, record), false, F("17th distinct slot refused"));
    test.expectEq(journalCount(), count, F("refusal precedes publication"));
    test.expectEq(storeWrite(0, record), true, F("repeated slot replaces index entry"));

    fresh();
    state = snapshot();
    test.expectEq(static_cast<uint8_t>(save(state)),
                  static_cast<uint8_t>(SaveStep::Done), F("overflow baseline checkpoint"));
    for (uint8_t i = 0; i < 17; ++i) {
        JournalRecord log = {};
        log.op = 2;
        log.slot = i;
        storeRecordEncode(record, log.payload);
        test.expectEq(journalAppend(log), true, F("crafted committed log prefix"));
    }
    test.expectEq(storeBoot(state), false, F("overflow log refuses gameplay"));
    test.expectEq(storeBootFailed(), true, F("boot index overflow fails closed"));
}

// Preserve the device-only status renderer coverage: the SAVING/FAILED glyphs
// are internal-flash and must render while save flash is busy.
inline void test_saveStatusRender(FxTest &test)
{
    using namespace save_fx_test_detail;
    fresh();
    SaveFile state = snapshot();
    test.expectEq(static_cast<uint8_t>(save(state)),
                  static_cast<uint8_t>(SaveStep::Done), F("status baseline save"));
    SaveController::drawStatus();
    expectPixel(test, 34, 27, 0, F("FAILED F glyph pixel"));
    expectPixel(test, 32, 27, 1, F("FAILED screen background"));

    FX::eraseSaveBlock(0); // unused save_main block forces a busy save flash
    const bool busyBeforeDraw = flash.busy();
    SaveController::begin(GameState_t::WORLD);
    test.expectEq(busyBeforeDraw, true, F("save flash busy before status draw"));
    test.expectEq(saveInProgress(), true, F("save remains active before status draw"));
    SaveController::drawStatus();
    test.expectEq(flash.busy(), true, F("save flash remains busy after status draw"));
    test.expectEq(saveInProgress(), true, F("save remains active after status draw"));
    expectPixel(test, 34, 27, 1, F("S first pixel paper"));
    expectPixel(test, 36, 27, 0, F("S second pixel ink"));
    expectPixel(test, 44, 27, 1, F("A first pixel paper"));
    expectPixel(test, 46, 27, 0, F("A second pixel ink"));
    expectPixel(test, 54, 27, 0, F("V first pixel ink"));
    expectPixel(test, 56, 27, 1, F("V second pixel paper"));
    expectPixel(test, 64, 27, 0, F("I first pixel ink"));
    expectPixel(test, 66, 27, 0, F("I second pixel ink"));
    expectPixel(test, 74, 27, 0, F("N first pixel ink"));
    expectPixel(test, 76, 27, 1, F("N second pixel paper"));
    expectPixel(test, 84, 27, 1, F("G first pixel paper"));
    expectPixel(test, 86, 27, 0, F("G second pixel ink"));
    FX::waitWhileBusy();
    test.expectEq(ReadFXu16(move_table), rawReadMoveTableFirst,
                  F("FX data read after save wait"));
}

inline void test_save(FxTest &test)
{
    test_saveJournalLayout(test);
    test_saveCheckpointCollection(test);
    test_saveCollectionCapacity(test);
    test_saveCheckpointCuts(test);
    test_saveJournalPublication(test);
    test_saveStatusRender(test);
}
