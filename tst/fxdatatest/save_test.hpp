#pragma once

#include <string.h>

#include "fxtest.hpp"
#include "frame_budget_test.hpp"
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
    record.attackMask = 2;
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

inline void test_saveBoxedInstanceHP(FxTest &test)
{
    using namespace save_fx_test_detail;
    fresh();
    SaveFile state = snapshot();
    StoreRecord ko = creature(27), duplicate = creature(30);
    ko.hpEncoded = 1; duplicate.hpEncoded = 19;
    uint8_t slot0 = 255, slot1 = 255;
    test.expectEq(storeAdd(ko, slot0), true, F("add KO instance"));
    test.expectEq(storeAdd(duplicate, slot1), true, F("add injured duplicate"));
    test.expectEq(storeInstanceHP(ko), 0, F("encoded one means KO"));
    test.expectEq(storeInstanceHP(duplicate), 18, F("duplicate HP decodes independently"));
    StoreRecord legacy = creature();
    Creature full;
    full.load(legacy);
    test.expectEq(storeInstanceHP(legacy), full.statlist.hp, F("legacy zero defaults to full"));
    test.expectEq(static_cast<uint8_t>(save(state)), static_cast<uint8_t>(SaveStep::Done),
                  F("boxed HP checkpoint completes"));
    test.expectEq(saveByte(save_store_a + 112 + 7), 1, F("KO encoding checkpointed"));
    test.expectEq(saveByte(save_store_a + 120 + 7), 19, F("duplicate encoding checkpointed"));
    SaveFile loaded = snapshot();
    test.expectEq(storeBoot(loaded), true, F("boxed HP checkpoint reboots"));
    StoreRecord restored = {};
    test.expectEq(storeRead(slot0, restored), true, F("KO instance reloads"));
    test.expectEq(storeInstanceHP(restored), 0, F("KO survives checkpoint"));
    test.expectEq(storeRead(slot1, restored), true, F("duplicate reloads"));
    test.expectEq(storeInstanceHP(restored), 18, F("duplicate HP survives checkpoint"));

    test.expectEq(storeAssign(0, slot0, 12), true, F("assign instance with current HP"));
    test.expectEq(static_cast<uint8_t>(save(loaded)), static_cast<uint8_t>(SaveStep::Done),
                  F("assignment snapshot publishes"));
    restored = creature(); // Legacy byte 7 is zero.
    test.expectEq(storeWrite(slot0, restored), true, F("append legacy record update"));
    test.expectEq(storeBoot(loaded), true, F("legacy update replays"));
    test.expectEq(loaded.partyHP[0], 12, F("legacy record preserves snapshot HP"));
    restored.hpEncoded = 8;
    test.expectEq(storeWrite(slot0, restored), true, F("append encoded record update"));
    test.expectEq(storeBoot(loaded), true, F("encoded update replays"));
    test.expectEq(loaded.partyHP[0], 7, F("record HP supersedes older snapshot"));
    test.expectEq(storeAssign(0, slot0, 5), true, F("append later party assignment"));
    test.expectEq(storeBoot(loaded), true, F("assignment authority replays"));
    test.expectEq(loaded.partyHP[0], 5, F("later assignment supersedes record HP"));
}

inline void test_saveBoxedHPPublicationCuts(FxTest &test)
{
    using namespace save_fx_test_detail;
    const int32_t cuts[] = {0, 4096, 4608, 4718, 4719, 4720, 4730, 8816, 8817};
    for (uint8_t c = 0; c < sizeof(cuts) / sizeof(cuts[0]); ++c) {
        fresh();
        SaveFile state = snapshot();
        StoreRecord member = creature(); member.hpEncoded = 13;
        uint8_t slot = 255;
        test.expectEq(storeAdd(member, slot), true, F("seed HP instance"));
        test.expectEq(storeAssign(0, slot, 12), true, F("seed snapshot HP"));
        test.expectEq(static_cast<uint8_t>(save(state)), static_cast<uint8_t>(SaveStep::Done),
                      F("baseline HP checkpoint"));
        member.hpEncoded = 8;
        test.expectEq(storeWrite(slot, member), true, F("publish newer HP to journal"));
        cutBegin(cuts[c]); save(state); cutEnd();
        SaveFile loaded = snapshot();
        test.expectEq(storeBoot(loaded), true, F("HP cut reboots"));
        test.expectEq(loaded.partyHP[0], 7, F("new HP survives publication cut"));
        StoreRecord restored = {};
        test.expectEq(storeRead(slot, restored), true, F("cut record readable"));
        test.expectEq(storeInstanceHP(restored), 7, F("record and party HP agree"));
        test.expectEq(storeBootFailed(), false, F("HP cut remains recoverable"));
    }

    fresh();
    SaveFile state = snapshot();
    StoreRecord member = creature(); member.hpEncoded = 13;
    uint8_t slot = 255;
    test.expectEq(storeAdd(member, slot), true, F("seed dirty-tail instance"));
    test.expectEq(storeAssign(0, slot, 12), true, F("seed dirty-tail HP"));
    test.expectEq(static_cast<uint8_t>(save(state)), static_cast<uint8_t>(SaveStep::Done),
                  F("dirty-tail baseline checkpoint"));
    member.hpEncoded = 8;
    cutBegin(15);
    test.expectEq(storeWrite(slot, member), false, F("torn HP record is unpublished"));
    cutEnd();
    test.expectEq(static_cast<uint8_t>(save(state)), static_cast<uint8_t>(SaveStep::Done),
                  F("explicit save clears dirty HP tail"));
    SaveFile loaded = snapshot();
    test.expectEq(storeBoot(loaded), true, F("dirty-tail save reboots"));
    test.expectEq(loaded.partyHP[0], 12, F("unpublished update preserves snapshot HP"));
    StoreRecord restored = {};
    test.expectEq(storeRead(slot, restored), true, F("dirty-tail record readable"));
    test.expectEq(storeInstanceHP(restored), 12, F("old encoded HP remains authoritative"));
}

inline void test_saveControllerCaptureHPCuts(FxTest &test)
{
    using namespace save_fx_test_detail;
    const int32_t cuts[] = {4719, 4720};
    for (uint8_t c = 0; c < sizeof(cuts) / sizeof(cuts[0]); ++c) {
        fresh();
        SaveFile state = snapshot();
        StoreRecord member = creature(); member.hpEncoded = 13;
        uint8_t slot = 255;
        test.expectEq(storeAdd(member, slot), true, F("seed full-log party instance"));
        test.expectEq(storeAssign(0, slot, 12), true, F("seed full-log snapshot HP"));
        test.expectEq(static_cast<uint8_t>(save(state)), static_cast<uint8_t>(SaveStep::Done),
                      F("full-log baseline checkpoint"));
        member.hpEncoded = 0;
        for (uint16_t i = 0; i < JOURNAL_CAPACITY; ++i) {
            JournalRecord log = {};
            log.seq = static_cast<uint8_t>(i);
            log.op = static_cast<uint8_t>(LogOp::StoreAdd);
            log.slot = slot;
            storeRecordEncode(member, log.payload);
            uint8_t bytes[JOURNAL_RECORD_BYTES];
            test.expectEq(journalEncode(log, bytes), true, F("encode full-journal record"));
            flash.writeBytes(save_log + i * JOURNAL_RECORD_BYTES, bytes, sizeof(bytes));
            FX::waitWhileBusy();
        }
        test.expectEq(journalCount(), JOURNAL_CAPACITY, F("journal contains all 256 records"));
        player = Player(); player.party[0].load(member); player.creatureHPs[0] = 7;
        cutBegin(cuts[c]);
        SaveController::begin(GameState_t::WORLD);
        test.expectEq(journalCount(), JOURNAL_CAPACITY,
                      F("HP-only capture does not consume full journal capacity"));
        for (uint16_t i = 0; i < 256 && saveInProgress(); ++i) {
            SaveController::advance();
            FX::waitWhileBusy();
        }
        cutEnd();
        SaveFile loaded = snapshot();
        test.expectEq(storeBoot(loaded), true, F("full-log capture cut reboots"));
        const uint8_t expected = cuts[c] >= 4720 ? 7 : 12;
        test.expectEq(loaded.partyHP[0], expected, F("capture HP follows checkpoint commit"));
        StoreRecord restored = {};
        test.expectEq(storeRead(slot, restored), true, F("full-log record readable"));
        if (cuts[c] >= 4720)
            test.expectEq(restored.hpEncoded, 8, F("committed capture mirrors record HP"));
    }

    const int32_t dirtyCuts[] = {4719, 4720};
    for (uint8_t c = 0; c < sizeof(dirtyCuts) / sizeof(dirtyCuts[0]); ++c) {
        fresh();
        SaveFile state = snapshot();
        StoreRecord member = creature(); member.hpEncoded = 13;
        uint8_t slot = 255;
        test.expectEq(storeAdd(member, slot), true, F("seed dirty-tail party instance"));
        test.expectEq(storeAssign(0, slot, 12), true, F("seed dirty-tail HP"));
        test.expectEq(static_cast<uint8_t>(save(state)), static_cast<uint8_t>(SaveStep::Done),
                      F("dirty-tail baseline checkpoint"));
        member.hpEncoded = 0;
        test.expectEq(storeWrite(slot, member), true, F("publish legacy prefix"));
        member.hpEncoded = 8; cutBegin(15);
        test.expectEq(storeWrite(slot, member), false, F("leave unpublished dirty tail"));
        cutEnd();
        player = Player(); player.party[0].load(member); player.creatureHPs[0] = 7;
        cutBegin(dirtyCuts[c]);
        SaveController::begin(GameState_t::WORLD);
        test.expectEq(journalCount(), 1, F("HP-only capture leaves dirty-tail prefix unchanged"));
        for (uint16_t i = 0; i < 256 && saveInProgress(); ++i) {
            SaveController::advance();
            FX::waitWhileBusy();
        }
        cutEnd();
        SaveFile loaded = snapshot();
        test.expectEq(storeBoot(loaded), true, F("dirty-tail capture cut reboots"));
        const uint8_t expected = dirtyCuts[c] >= 4720 ? 7 : 12;
        test.expectEq(loaded.partyHP[0], expected, F("dirty-tail capture follows commit"));
        StoreRecord restored = {};
        test.expectEq(storeRead(slot, restored), true, F("dirty-tail record readable"));
        if (dirtyCuts[c] >= 4720)
            test.expectEq(restored.hpEncoded, 8, F("dirty-tail commit mirrors captured HP"));
    }
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
    // Byte-write-budget boundaries of one save with the real device write-limit
    // seam (the erase completes atomically, then the budget is reduced):
    //   0    before the inactive erase; nothing written
    //   4096 inactive erase complete; no header yet
    //   4608 mid record streaming; no header yet
    //   4718 header written, commit bytes absent
    //   4719 one of two commit bytes present
    //   4720 commit complete -> new checkpoint authoritative
    //   4730 cleanup-bit clear blocked after the journal erase
    //   8816 journal erased, cleanup bit still pending
    //   8817 cleanup bit cleared
    // (The 4608 below is the separate FIRST-save case, not prior-checkpoint 4608.)
    const int32_t cuts[] = {0, 4096, 4608, 4718, 4719, 4720, 4730, 8816, 8817};
    const StoreRecord expected = creature();
    const uint8_t emptyMask = 0;
    uint8_t expectedSnapshot[96];
    saveFileEncode(snapshot(0), 0, expectedSnapshot, 96);
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
        const uint16_t expectedLocation = cuts[c] >= 4720 ? 22 : 11;
        test.expectEq(loaded.playerLocation, expectedLocation,
                      F("commit markers define snapshot authority"));
        expectedSnapshot[0] = static_cast<uint8_t>(expectedLocation);
        expectedSnapshot[1] = static_cast<uint8_t>(expectedLocation >> 8);
        uint8_t loadedSnapshot[96];
        saveFileEncode(loaded, 0, loadedSnapshot, 96);
        test.expectEq(memcmp(loadedSnapshot, expectedSnapshot, 96), 0,
                      F("complete 96-byte snapshot matches authority"));
        for (uint8_t i = 0; i < 64; ++i) {
            StoreRecord got = {};
            test.expectEq(storeRead(i, got), true, F("cut slot readable"));
            const bool added = i == slot;
            test.expectEq(got.id, added ? expected.id : static_cast<uint8_t>(255),
                          F("cut record id"));
            test.expectEq(got.exp, added ? expected.exp : static_cast<uint16_t>(0),
                          F("cut record exp"));
            test.expectEq(got.attackMask, added ? expected.attackMask : emptyMask,
                          F("cut record mask"));
        }
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
inline void test_savePlayerStore(FxTest &test)
{
    using namespace save_fx_test_detail;
    fresh();
    StoreRecord member = creature(126);
    uint8_t slot0 = 255;
    test.expectEq(storeAdd(member, slot0), true, F("seed party instance"));
    StoreRecord dup = creature(130);
    uint8_t slot1 = 255;
    test.expectEq(storeAdd(dup, slot1), true, F("seed duplicate instance"));
    test.expectEq(storeAssign(0, slot0, 17), true, F("party references instance"));
    player = Player();
    player.party[0].load(member);
    player.creatureHPs[0] = 17;
    player.creatureHPs[2] = 33; // stale live HP on an unassigned slot must not persist
    SaveController::begin(GameState_t::WORLD);
    for (uint16_t i = 0; i < 256 && saveInProgress(); ++i) {
        SaveController::advance();
        FX::waitWhileBusy();
    }
    test.expectEq(gameState.state == GameState_t::WORLD, true, F("initial save completes"));
    SaveFile initialCheckpoint = {};
    test.expectEq(storeBoot(initialCheckpoint), true, F("initial checkpoint boots"));
    test.expectEq(initialCheckpoint.partyHP[2], 0, F("empty HP is zero before load"));
    player = Player();
    test.expectEq(SaveController::load(), true, F("non-threshold checkpoint reloads"));
    StoreRecord stored = {};
    test.expectEq(storeRead(slot0, stored), true, F("stored instance readable"));
    test.expectEq(stored.exp, 126, F("non-threshold XP preserved when level unchanged"));
    player.party[0].level = 6;
    SaveController::begin(GameState_t::WORLD);
    for (uint16_t i = 0; i < 256 && saveInProgress(); ++i) {
        SaveController::advance();
        FX::waitWhileBusy();
    }
    test.expectEq(gameState.state == GameState_t::WORLD, true, F("level-change save completes"));
    player = Player();
    test.expectEq(SaveController::load(), true, F("level-change checkpoint reloads"));
    test.expectEq(storeRead(slot0, stored), true, F("leveled instance readable"));
    test.expectEq(stored.exp, levelExpThreshold(6), F("level change rewrites only its threshold"));
    test.expectEq(storeRead(slot1, stored), true, F("duplicate readable"));
    test.expectEq(stored.exp, 130, F("duplicate keeps its exact fractional XP"));
    SaveFile loaded = {};
    test.expectEq(storeBoot(loaded), true, F("player/store checkpoint boots"));
    test.expectEq(loaded.partySlots[0], slot0, F("party slot restored"));
    test.expectEq(loaded.partyHP[0], 17, F("explicit party HP restored"));
    test.expectEq(loaded.partySlots[2], 255, F("empty party slot stays 255"));
    test.expectEq(loaded.partyHP[2], 0, F("empty party slot stores zero HP"));
    player = Player();
    test.expectEq(SaveController::load(), true, F("shipping load works"));
    test.expectEq(player.party[0].id, member.id, F("party id restored"));
    test.expectEq(player.creatureHPs[0], 17, F("party HP restored"));
    test.expectEq(player.party[2].id, 255, F("empty entry restores id 255"));
    test.expectEq(player.creatureHPs[2], 0, F("empty entry restores HP zero"));
}

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
    // The packed move table is retired; the save path issues no move-table read.
}

// Worst-case frame-time proof on the shipping save chain. Shipping drives the
// save one SaveController::advance() step per frame: the sector erase stays in
// flight and saveStepAdvance() returns while flash is busy, so this times that
// exact frame (advance + status render + the real FX display). The deliberately
// slowed frame is the permanent negative control for the budget assertion.
inline void test_saveFrameBudget(FxTest &test)
{
    using namespace save_fx_test_detail;
    fresh();
    gameState.playerLocation = 0x1234;
    StoreRecord record = creature();
    uint8_t slot = 255;
    test.expectEq(storeAdd(record, slot), true, F("frame-budget seed instance"));
    test.expectEq(storeAssign(0, slot, 15), true, F("frame-budget seed assignment"));

    SaveController::begin(GameState_t::WORLD);
    frame_budget_test_detail::FrameBudget budget;

    // Negative control: arm one real frame, verify detection, then restore.
    budget.reset();
    budget.armSlowFrame();
    budget.begin();
    SaveController::advance();
    SaveController::drawStatus();
    FX::display(CLEAR_BUFFER);
    budget.end();
    budget.expectDetectsSlowFrame(
        test, F("negative control: slowed save frame exceeds budget"));

    budget.reset();
    for (uint16_t frame = 0; frame < 256 && saveInProgress(); ++frame) {
        budget.begin();
        SaveController::advance();
        SaveController::drawStatus();
        FX::display(CLEAR_BUFFER);
        budget.end();
    }
    test.expectEq(saveInProgress(), false, F("save frame loop completes"));
    budget.expectWithinBudget(test, F("worst save frame <= 19230 us"));
}

inline void test_save(FxTest &test)
{
    test_saveJournalLayout(test);
    test_saveCheckpointCollection(test);
    test_saveBoxedInstanceHP(test);
    test_saveBoxedHPPublicationCuts(test);
    test_saveControllerCaptureHPCuts(test);
    test_saveCollectionCapacity(test);
    test_saveCheckpointCuts(test);
    test_saveJournalPublication(test);
    test_savePlayerStore(test);
    test_saveStatusRender(test);
    test_saveFrameBudget(test);
}
