#pragma once
#include <string.h>
#include <stddef.h>
#include "test.hpp"
#include "../src/save/Compaction.hpp"
#include "../src/fxdata.h"
#include "../src/save/FlashBackend.hpp"
#include "../src/save/Journal.hpp"
#include "../src/save/SaveController.hpp"
#include "../src/engine/battle/BattlePresets.hpp"
#include "../src/creature/Creature.hpp"
#include "../src/creature/LevelCurve.hpp"
#include "../src/creature/Evolution.hpp"
#include "../src/flags/flag_bit_array.hpp"
#include "../src/globals.hpp"
#include "../src/item/Inventory.hpp"
#include "../src/item/KeyItems.hpp"
#include "../src/plants/PlantGamestate.hpp"

namespace save_test_detail {
inline SaveFile snapshot(uint16_t location=0) {
    SaveFile state={};state.playerLocation=location;memset(state.partySlots,255,3);return state;
}
inline SaveStep save(SaveFile &state) {
    saveBegin();SaveStep result=SaveStep::Idle;
    for(uint16_t i=0;i<256 && saveInProgress();++i) result=saveStepAdvance(state);
    return result;
}
inline StoreRecord creature(uint16_t exp=27) { return {0,exp,2,{0,0,0},0}; }
inline void fresh() { flashFakeReset();storeFresh(); }

// Independent Fletcher-16 oracle for the 12-byte normalized header. Reimplements
// the bound-and-wrap rule so the test does not reuse the shipping checksum path.
inline uint16_t fletcher16(const uint8_t bytes[12]) {
    uint8_t a=0,b=0;
    for(uint8_t i=0;i<12;++i){
        uint16_t n=static_cast<uint16_t>(a)+bytes[i];
        a=n>=255?static_cast<uint8_t>(n-255):static_cast<uint8_t>(n);
        n=static_cast<uint16_t>(b)+a;
        b=n>=255?static_cast<uint8_t>(n-255):static_cast<uint8_t>(n);
    }
    return static_cast<uint16_t>((static_cast<uint16_t>(b)<<8)|a);
}
// Test-only backend seeding: rewrite one committed checkpoint's generation and
// re-derive its header checksum so a valid current-format fixture can start at
// 0xFFFF. Only test storage changes; no production hook or API is added.
inline void seedGeneration(uint16_t base,uint16_t generation) {
    uint8_t header[14];
    memcpy(header,flashFakeData()+base,sizeof(header));
    header[4]=static_cast<uint8_t>(generation);
    header[5]=static_cast<uint8_t>(generation>>8);
    uint8_t check[12];memcpy(check,header,sizeof(check));check[9]=1;
    const uint16_t sum=fletcher16(check);
    header[12]=static_cast<uint8_t>(sum);header[13]=static_cast<uint8_t>(sum>>8);
    flashFakeSetBytes(base,header,sizeof(header));
}
inline void rewriteBodyChecksum(uint16_t base) {
    uint8_t a=0,b=0;
    for(uint16_t at=16;at<624;++at) {
        const uint16_t n1=static_cast<uint16_t>(a)+flashFakeData()[base+at];
        a=n1>=255?n1-255:n1;
        const uint16_t n2=static_cast<uint16_t>(b)+a;
        b=n2>=255?n2-255:n2;
    }
    const uint8_t body[2]={a,b};flashFakeSetBytes(base+10,body,sizeof(body));
    const uint8_t *header=flashFakeData()+base;
    const uint16_t generation=static_cast<uint16_t>(header[4])|
                              (static_cast<uint16_t>(header[5])<<8);
    seedGeneration(base,generation);
}
}
inline void CheckpointCollectionTest(TestSuite &suite) {
    using namespace save_test_detail;Test test(__func__);fresh();
    SaveFile state=snapshot(0x1234);StoreRecord record=creature();uint8_t slot=255;
    test.assert(storeAdd(record,slot),true,"first instance add");test.assert(slot,0,"first stable slot");
    test.assert(storeAssign(0,slot,17),true,"party references instance");
    record.exp=125;test.assert(storeAdd(record,slot),true,"duplicate species add");test.assert(slot,1,"duplicate owns separate slot");
    test.assert(storeRead(63,record),true,"last empty slot readable");test.assert(record.id,255,"empty is canonical tombstone");
    test.assert(storeRead(64,record),false,"out of range refused");
    test.assert(save(state),SaveStep::Done,"coherent save completes");
    SaveFile loaded=snapshot();test.assert(storeBoot(loaded),true,"power cycle selects checkpoint");
    test.assert(loaded.playerLocation,0x1234,"snapshot location shares checkpoint");
    test.assert(loaded.partySlots[0],0,"party stable reference restored");
    test.assert(loaded.partyHP[0],17,"assignment HP restored explicitly");
    test.assert(storeRead(1,record),true,"duplicate restored");test.assert(record.exp,125,"duplicate experience retained");
    Creature instance;instance.load(record);test.assert(instance.level,levelFromExp(125),"derived level restored");
    test.assert(journalCount(),0,"journal erased after coherent publication");
    test.assert(storeAssign(0,1,11),true,"new occupant carries explicit HP");
    test.assert(storePartyHP(0),11,"old occupant HP is replaced");
    test.assert(storeAssign(1,1,11),false,"duplicate party reference refused");
    test.assert(storeAssign(0,255,1),false,"unassignment requires zero HP");
    test.assert(storeAssign(0,1,255),false,"HP above derived maximum refused");
    test.assert(storeBoot(loaded),true,"assignment journal replays on reboot");
    test.assert(loaded.partySlots[0],1,"new party occupant restored");
    test.assert(loaded.partyHP[0],11,"journal restores new occupant HP");
    test.assert(storeRemove(1),false,"party slot cannot be removed");
    test.assert(storeRecordAddr(8192,63),8192+616,"slot63 starts at624 minus8");
    test.assert(flashFakeData()[8192+9],0,"cleanup bit clear after journal verification");
    suite.addTest(test);
}
inline void BoxedInstanceHPTest(TestSuite &suite) {
    using namespace save_test_detail;Test test(__func__);fresh();
    SaveFile state=snapshot();StoreRecord first=creature(27),duplicate=creature(30);
    uint8_t a=255,b=255;
    first.hpEncoded=1;duplicate.hpEncoded=19; // KO and 18 HP, distinct instances.
    test.assert(storeAdd(first,a),true,"add KO instance");
    test.assert(storeAdd(duplicate,b),true,"add injured duplicate");
    test.assert(a,0,"first stable instance slot");test.assert(b,1,"duplicate stable instance slot");
    test.assert(storeInstanceHP(first),0,"encoded one represents KO");
    test.assert(storeInstanceHP(duplicate),18,"encoded HP subtracts one");
    StoreRecord legacy=creature(27);
    Creature full;full.load(legacy);
    test.assert(storeInstanceHP(legacy),full.hp,"legacy zero byte defaults to full HP");
    test.assert(save(state),SaveStep::Done,"boxed HP checkpoint completes");
    test.assert(flashFakeData()[8192+112+7],1,"checkpoint persists KO encoding");
    test.assert(flashFakeData()[8192+120+7],19,"checkpoint persists duplicate encoding");
    SaveFile loaded=snapshot();test.assert(storeBoot(loaded),true,"boxed HP checkpoint reboots");
    StoreRecord restored={};test.assert(storeRead(a,restored),true,"KO instance reloads");
    test.assert(storeInstanceHP(restored),0,"KO remains zero after reboot");
    test.assert(storeRead(b,restored),true,"duplicate reloads");
    test.assert(storeInstanceHP(restored),18,"duplicate retains its own HP");

    // A nonzero journal record is newer than snapshot HP; byte zero remains
    // compatible with legacy party snapshots and later PartyAssign wins.
    test.assert(storeAssign(0,a,12),true,"assign KO instance with explicit current HP");
    test.assert(save(loaded),SaveStep::Done,"assignment snapshot publishes");
    restored=creature(27); // old caller format, byte 7 zero
    test.assert(storeWrite(a,restored),true,"legacy record update appended");
    test.assert(storeBoot(loaded),true,"legacy record replay succeeds");
    test.assert(loaded.partyHP[0],12,"legacy zero preserves snapshot HP");
    restored.hpEncoded=8; // actual 7 HP
    test.assert(storeWrite(a,restored),true,"new encoded HP update appended");
    test.assert(storeBoot(loaded),true,"encoded HP replay succeeds");
    test.assert(loaded.partyHP[0],7,"record HP supersedes older snapshot HP");
    test.assert(storeAssign(0,a,5),true,"later assignment carries newer HP");
    test.assert(storeBoot(loaded),true,"assignment authority replays");
    test.assert(loaded.partyHP[0],5,"later PartyAssign supersedes record HP");

    // Reassign the party slot between duplicate instances after mirroring the
    // outgoing member's KO HP, then checkpoint and recover both exact values.
    fresh();state=snapshot();first=creature(27);duplicate=creature(30);
    first.hpEncoded=1;duplicate.hpEncoded=19;a=b=255;
    test.assert(storeAdd(first,a),true,"seed outgoing KO duplicate");
    test.assert(storeAdd(duplicate,b),true,"seed incoming injured duplicate");
    test.assert(storeAssign(0,a,0),true,"assign outgoing KO member");
    test.assert(save(state),SaveStep::Done,"duplicate swap baseline");
    test.assert(storeWrite(a,first),true,"mirror outgoing KO HP before reassignment");
    test.assert(storeAssign(0,b,18),true,"assign incoming duplicate with its HP");
    test.assert(save(state),SaveStep::Done,"duplicate swap checkpoint");
    test.assert(storeBoot(loaded),true,"duplicate swap reboots");
    test.assert(loaded.partySlots[0],b,"incoming duplicate remains assigned");
    test.assert(loaded.partyHP[0],18,"incoming duplicate retains injured HP");
    test.assert(storeRead(a,restored),true,"outgoing duplicate survives swap");
    test.assert(storeInstanceHP(restored),0,"outgoing duplicate retains KO HP");
    test.assert(storeRead(b,restored),true,"incoming duplicate record survives swap");
    test.assert(storeInstanceHP(restored),18,"incoming duplicate retains independent HP");

    // Build a valid old-format v3 checkpoint by clearing its party record's
    // HP byte and re-deriving both integrity checks. Snapshot HP stays authority.
    StoreRecord member;
    for(uint8_t hpCase=0;hpCase<2;++hpCase) {
        fresh();state=snapshot();member=creature(27);member.hpEncoded=hpCase?1:13;
        a=255;test.assert(storeAdd(member,a),true,"seed legacy checkpoint party");
        test.assert(storeAssign(0,a,hpCase?0:12),true,"seed legacy snapshot party HP");
        test.assert(save(state),SaveStep::Done,"legacy checkpoint baseline");
        const uint8_t zero=0;flashFakeSetBytes(8192+112+7,&zero,1);rewriteBodyChecksum(8192);
        uint8_t check[12],header[16];memcpy(header,flashFakeData()+8192,16);
        for(uint8_t i=0;i<12;++i) check[i]=header[i];check[9]=1;
        test.assert(fletcher16(check),static_cast<uint16_t>(header[12]|(header[13]<<8)),"legacy header checksum recomputed");
        loaded=snapshot();test.assert(storeBoot(loaded),true,"legacy zero-byte checkpoint validates");
        test.assert(loaded.partyHP[0],hpCase?0:12,"legacy snapshot injury or KO persists");
        player=Player();test.assert(SaveController::load(),true,"legacy party loads through controller");
        test.assert(player.creatureHPs[0],hpCase?0:12,"legacy party HP remains authoritative");
    }
    suite.addTest(test);
}
inline void BoxedHPPublicationCutsTest(TestSuite &suite) {
    using namespace save_test_detail;Test test(__func__);
    const int32_t cuts[]={0,4096,4608,4718,4719,4720,4730,8816,8817};
    for(int32_t cut:cuts) {
        fresh();SaveFile state=snapshot();StoreRecord member=creature();
        member.hpEncoded=13;uint8_t slot=255;
        test.assert(storeAdd(member,slot),true,"seed party instance with HP");
        test.assert(storeAssign(0,slot,12),true,"seed snapshot HP");
        test.assert(save(state),SaveStep::Done,"baseline HP checkpoint");
        member.hpEncoded=8;
        test.assert(storeWrite(slot,member),true,"publish newer instance HP to journal");
        flashFakeSetWriteLimit(cut);save(state);flashFakeSetWriteLimit(-1);
        SaveFile loaded=snapshot();test.assert(storeBoot(loaded),true,"HP cut reboots");
        test.assert(loaded.partyHP[0],7,"new record HP survives every publication cut");
        StoreRecord restored={};test.assert(storeRead(slot,restored),true,"cut record readable");
        test.assert(storeInstanceHP(restored),7,"record HP agrees with recovered party HP");
        test.assert(storeBootFailed(),false,"HP cut does not fail closed");
    }
    // A dirty tail cannot publish an HP mutation; explicit save folds only the
    // valid prefix, clears the journal, and preserves the prior HP authority.
    fresh();SaveFile state=snapshot();StoreRecord member=creature();member.hpEncoded=13;
    uint8_t slot=255;test.assert(storeAdd(member,slot),true,"seed dirty-tail instance");
    test.assert(storeAssign(0,slot,12),true,"seed dirty-tail party HP");
    test.assert(save(state),SaveStep::Done,"dirty-tail baseline checkpoint");
    member.hpEncoded=8;flashFakeSetWriteLimit(15);
    test.assert(storeWrite(slot,member),false,"torn HP record remains unpublished");
    flashFakeSetWriteLimit(-1);
    test.assert(save(state),SaveStep::Done,"explicit save clears dirty HP tail");
    SaveFile loaded=snapshot();test.assert(storeBoot(loaded),true,"dirty-tail save reboots");
    test.assert(loaded.partyHP[0],12,"unpublished HP update does not replace old snapshot");
    StoreRecord restored={};test.assert(storeRead(slot,restored),true,"dirty-tail record readable");
    test.assert(storeInstanceHP(restored),12,"dirty-tail preserves old encoded HP");
    suite.addTest(test);
}
inline void SaveControllerCaptureHPCutsTest(TestSuite &suite) {
    using namespace save_test_detail;Test test(__func__);
    const int32_t cuts[]={4719,4720};
    for(int32_t cut:cuts) {
        fresh();SaveFile state=snapshot();StoreRecord member=creature();member.hpEncoded=13;
        uint8_t slot=255;test.assert(storeAdd(member,slot),true,"seed full-log party instance");
        test.assert(storeAssign(0,slot,12),true,"seed full-log snapshot HP");
        test.assert(save(state),SaveStep::Done,"full-log baseline checkpoint");
        member.hpEncoded=0;
        for(uint16_t i=0;i<JOURNAL_CAPACITY;++i) {
            JournalRecord log={};log.seq=static_cast<uint8_t>(i);
            log.op=static_cast<uint8_t>(LogOp::StoreAdd);log.slot=slot;
            storeRecordEncode(member,log.payload);
            uint8_t bytes[JOURNAL_RECORD_BYTES];
            test.assert(journalEncode(log,bytes),true,"encode full-journal legacy update");
            flashFakeSetBytes(save_log+i*JOURNAL_RECORD_BYTES,bytes,sizeof(bytes));
        }
        test.assert(journalCount(),JOURNAL_CAPACITY,"journal reaches all 256 records");
        player=Player();player.party[0].load(member);player.creatureHPs[0]=7;
        flashFakeSetWriteLimit(cut);
        SaveController::begin(GameState_t::WORLD);
        test.assert(journalCount(),JOURNAL_CAPACITY,"HP-only capture adds no journal record at capacity");
        for(uint16_t i=0;i<256 && saveInProgress();++i) SaveController::advance();
        flashFakeSetWriteLimit(-1);
        SaveFile loaded=snapshot();test.assert(storeBoot(loaded),true,"full-log capture cut reboots");
        const uint8_t expected=cut>=4720?7:12;
        test.assert(loaded.partyHP[0],expected,"capture HP publishes with checkpoint commit");
        StoreRecord restored={};test.assert(storeRead(slot,restored),true,"full-log record readable");
        if(cut>=4720) test.assert(restored.hpEncoded,8,"committed capture mirrors record HP");
    }
    // A valid prefix plus dirty tail blocks SaveController's HP append. The
    // compacted snapshot/record pair still publishes atomically.
    const int32_t dirtyCuts[]={4719,4720};
    for(int32_t cut:dirtyCuts) {
        fresh();SaveFile state=snapshot();StoreRecord member=creature();member.hpEncoded=13;
        uint8_t slot=255;test.assert(storeAdd(member,slot),true,"seed dirty-tail party instance");
        test.assert(storeAssign(0,slot,12),true,"seed dirty-tail snapshot HP");
        test.assert(save(state),SaveStep::Done,"dirty-tail baseline checkpoint");
        member.hpEncoded=0;test.assert(storeWrite(slot,member),true,"publish legacy prefix");
        member.hpEncoded=8;flashFakeSetWriteLimit(15);
        test.assert(storeWrite(slot,member),false,"leave dirty tail before save capture");
        flashFakeSetWriteLimit(-1);
        player=Player();player.party[0].load(member);player.creatureHPs[0]=7;
        flashFakeSetWriteLimit(cut);SaveController::begin(GameState_t::WORLD);
        test.assert(journalCount(),1,"HP-only capture adds no journal record behind dirty tail");
        for(uint16_t i=0;i<256 && saveInProgress();++i) SaveController::advance();
        flashFakeSetWriteLimit(-1);
        SaveFile loaded=snapshot();test.assert(storeBoot(loaded),true,"dirty-tail capture cut reboots");
        const uint8_t expected=cut>=4720?7:12;
        test.assert(loaded.partyHP[0],expected,"dirty-tail capture obeys checkpoint authority");
        StoreRecord restored={};test.assert(storeRead(slot,restored),true,"dirty-tail capture record readable");
        if(cut>=4720) test.assert(restored.hpEncoded,8,"dirty-tail candidate mirrors live HP");
    }
    suite.addTest(test);
}
inline void CollectionCapacityTest(TestSuite &suite) {
    using namespace save_test_detail;Test test(__func__);fresh();SaveFile state=snapshot();
    StoreRecord record=creature();uint8_t slot=255;
    for(uint8_t i=0;i<64;++i) {
        test.assert(storeAdd(record,slot),true,"bounded instance add");test.assert(slot,i,"stable allocation");
        if(i%16==15) test.assert(save(state),SaveStep::Done,"explicit batch save");
    }
    test.assert(storeAdd(record,slot),false,"full64 refuses overwrite");
    test.assert(storeBoot(state),true,"full checkpoint loads");
    test.assert(storeRead(63,record),true,"last slot restores");test.assert(record.id,0,"slot63 occupied");
    test.assert(storeRemove(63),true,"unreferenced last slot tombstone");
    test.assert(storeAdd(record,slot),true,"tombstone reused");test.assert(slot,63,"no slot squeeze");
    suite.addTest(test);
}
inline void CheckpointCutTest(TestSuite &suite) {
    using namespace save_test_detail;Test test(__func__);
    // Byte-write-budget boundaries of one save (erase 4096 + snapshot 96 +
    // records 512 + header 14 + commit 2 + journal-erase 4096 + clear 1 = 8817):
    //   0    before the inactive erase; nothing written
    //   4096 inactive erase complete; no header yet
    //   4608 mid record streaming; no header yet
    //   4718 header written, commit bytes absent
    //   4719 one of two commit bytes present
    //   4720 commit complete -> new checkpoint authoritative
    //   4730 cleanup-bit clear still blocked after the journal erase budget
    //   8816 journal erased, cleanup bit still pending
    //   8817 cleanup bit cleared
    const int32_t cuts[]={0,4096,4608,4718,4719,4720,4730,8816,8817};
    const uint8_t emptyMask=0;
    uint8_t expectedSnapshot[96];
    saveFileEncode(snapshot(0),0,expectedSnapshot,96);
    for(int32_t cut:cuts) {
        fresh();SaveFile state=snapshot(11);test.assert(save(state),SaveStep::Done,"baseline checkpoint");
        StoreRecord record=creature();uint8_t slot;test.assert(storeAdd(record,slot),true,"postcheckpoint mutation");
        state.playerLocation=22;flashFakeSetWriteLimit(cut);save(state);flashFakeSetWriteLimit(-1);
        SaveFile loaded=snapshot();test.assert(storeBoot(loaded),true,"cut reboots into valid state");
        const uint16_t expectedLocation=cut>=4720 ? 22 : 11;
        test.assert(loaded.playerLocation,expectedLocation,"commit markers define snapshot authority");
        expectedSnapshot[0]=static_cast<uint8_t>(expectedLocation);
        expectedSnapshot[1]=static_cast<uint8_t>(expectedLocation>>8);
        uint8_t loadedSnapshot[96];saveFileEncode(loaded,0,loadedSnapshot,96);
        test.assert(memcmp(loadedSnapshot,expectedSnapshot,96),0,"complete 96-byte snapshot matches authority");
        for(uint8_t i=0;i<64;++i) {
            StoreRecord got;test.assert(storeRead(i,got),true,"cut slot readable");
            const bool added=i==slot;
            test.assert(got.id,added?record.id:static_cast<uint8_t>(255),"cut record id");
            test.assert(got.exp,added?record.exp:static_cast<uint16_t>(0),"cut record exp");
            test.assert(got.attackMask,added?record.attackMask:emptyMask,"cut record mask");
        }
        test.assert(storeBootFailed(),false,"cut cleanup completed");
    }
    fresh();SaveFile state=snapshot();flashFakeSetWriteLimit(4608);save(state);flashFakeSetWriteLimit(-1);
    test.assert(storeBoot(state),false,"interrupted first save resets safely");test.assert(storeBootFailed(),false,"no committed save existed");
    test.assert(save(state),SaveStep::Done,"first save retried after reset");
    uint8_t bad=4;flashFakeSetBytes(8192+2,&bad,1);
    test.assert(storeBoot(state),false,"invalid committed header rejected");test.assert(storeBootFailed(),true,"committed corruption fails closed");
    suite.addTest(test);
}
inline void JournalDecodeTest(TestSuite &suite) {
    Test test(__func__);
    uint8_t bytes[JOURNAL_RECORD_BYTES];
    memset(bytes, 0xff, sizeof(bytes));
    JournalRecord decoded = {};
    test.assert(journalDecode(bytes, decoded), false, "blank record is not committed");
    bytes[3] = 0x27;
    test.assert(journalDecode(bytes, decoded), false, "dirty record without marker is not committed");
    JournalRecord authored = {};
    authored.seq = 19;
    authored.op = static_cast<uint8_t>(LogOp::StoreAdd);
    authored.slot = 0x1234;
    authored.payload[0] = 0x5a;
    uint8_t committed[JOURNAL_RECORD_BYTES];
    test.assert(journalEncode(authored, committed), true, "record encoder publishes marker");
    test.assert(journalDecode(committed, decoded), true, "committed record decodes");
    test.assert(decoded.seq, authored.seq, "decoded sequence matches");
    test.assert(decoded.op, authored.op, "decoded operation matches");
    test.assert(decoded.slot, authored.slot, "decoded slot matches");
    test.assert(decoded.payload[0], authored.payload[0], "decoded payload matches");
    committed[14] ^= 1;
    test.assert(journalDecode(committed, decoded), false, "bad checksum is rejected");
    suite.addTest(test);
}
inline void JournalPublicationTest(TestSuite &suite) {
    using namespace save_test_detail;Test test(__func__);fresh();
    StoreRecord record=creature();flashFakeSetWriteLimit(15);
    test.assert(storeWrite(0,record),false,"body without marker is unpublished");flashFakeSetWriteLimit(-1);
    test.assert(journalCount(),0,"torn marker stops replay");
    test.assert(storeWrite(0,record),false,"dirty tail not overwritten");
    SaveFile state=snapshot();test.assert(save(state),SaveStep::Done,"explicit save clears torn tail");
    for(uint8_t i=0;i<16;++i) test.assert(storeWrite(i,record),true,"pending index bounded write");
    const uint16_t count=journalCount();test.assert(storeWrite(16,record),false,"17th distinct slot refused");
    test.assert(journalCount(),count,"refusal precedes publication");
    test.assert(storeWrite(0,record),true,"repeated slot replaces index entry");
    fresh();state=snapshot();test.assert(save(state),SaveStep::Done,"overflow baseline checkpoint");
    for(uint8_t i=0;i<17;++i) {
        JournalRecord log={};log.op=2;log.slot=i;storeRecordEncode(record,log.payload);
        test.assert(journalAppend(log),true,"crafted committed log prefix");
    }
    test.assert(storeBoot(state),false,"overflow log refuses gameplay");
    test.assert(storeBootFailed(),true,"boot index overflow fails closed");
    suite.addTest(test);
}
inline void ShippingSaveRestoreTest(TestSuite &suite) {
    using namespace save_test_detail;Test test(__func__);fresh();player.basic();SaveController::seedParty();
    gameState.playerLocation=0x4321;player.creatureHPs[0]=17;
    uint8_t stages[8];memset(stages,0xe4,8);plants.plantStages.restore(stages);
    plants.plantPairs[0].restore(0xa9);plants.plantPairs[1].restore(0x3c);plants.ticker=91;
    SaveController::begin(GameState_t::WORLD);
    for(uint16_t i=0;i<256 && saveInProgress();++i) SaveController::advance();
    gameState.playerLocation=0;player.creatureHPs[0]=0;
    plants.plantStages=PlantStage();plants.plantPairs[0].restore(0);plants.ticker=0;
    test.assert(SaveController::load(),true,"shipping load works");
    test.assert(gameState.playerLocation,0x4321,"shipping location restored");
    test.assert(player.creatureHPs[0],17,"shipping HP restored");
    test.assert(player.party[0].level,31,"party level reconstructed");
    plants.plantStages.serialize(stages);
    for(uint8_t i=0;i<8;++i) test.assert(stages[i],0xe4,"canonical plant stage bytes restored");
    test.assert(plants.plantStages.getStage(31),3,"highest packed stage restored");
    test.assert(plants.plantPairs[0].serialize(),0xa9,"canonical plant pair restored");
    test.assert(plants.plantPairs[1].serialize(),0x3c,"second plant pair restored");
    test.assert(plants.ticker,91,"plant ticker restored");suite.addTest(test);
}
inline void SaveFlagExtentTest(TestSuite &suite) {
    using namespace save_test_detail;Test test(__func__);fresh();player.basic();SaveController::seedParty();
    uint8_t flagsAndNeighbor[sizeof(FLAG_BIT_ARRAY) + 8] = {};
    flagsAndNeighbor[0] = 0x5a;
    flagsAndNeighbor[sizeof(FLAG_BIT_ARRAY)] = 0xc7;
    uint8_t *originalFlags = gameState.flags;
    gameState.flags = flagsAndNeighbor;
    SaveController::begin(GameState_t::WORLD);
    for(uint16_t i=0;i<256 && saveInProgress();++i) SaveController::advance();
    SaveFile stored = {};
    test.assert(saveFileLoad(stored),true,"flag snapshot commits with adjacent byte set");
    test.assert(stored.flags[0],0x5a,"generated flag byte is captured");
    for(uint8_t i=sizeof(FLAG_BIT_ARRAY);i<sizeof(stored.flags);++i)
        test.assert(stored.flags[i],0,"unused save flag bytes stay canonical zero");
    flagsAndNeighbor[0] = 0;
    test.assert(SaveController::load(),true,"flag snapshot loads");
    test.assert(flagsAndNeighbor[0],0x5a,"generated flag byte is restored");
    test.assert(flagsAndNeighbor[sizeof(FLAG_BIT_ARRAY)],0xc7,
                "neighbor byte is unchanged by restore");
    gameState.flags = originalFlags;

    FLAG_BIT_ARRAY[0] = 0x69;
    SaveController::begin(GameState_t::WORLD);
    for(uint16_t i=0;i<256 && saveInProgress();++i) SaveController::advance();
    test.assert(saveFileLoad(stored),true,"generated flag pointer snapshot commits");
    for(uint8_t i=sizeof(FLAG_BIT_ARRAY);i<sizeof(stored.flags);++i)
        test.assert(stored.flags[i],0,"generated pointer leaves reserved flag bytes zero");
    FLAG_BIT_ARRAY[0] = 0;
    test.assert(SaveController::load(),true,"generated flag pointer snapshot loads");
    test.assert(FLAG_BIT_ARRAY[0],0x69,"real generated flag byte round-trips");
    suite.addTest(test);
}
// Force the committed generation to 0xFFFF on a real, valid current-format
// checkpoint, then save again and prove boot selects the wrapped generation 0
// as the newer checkpoint (signed 16-bit adjacency), not just test arithmetic.
inline void GenerationWrapTest(TestSuite &suite) {
    using namespace save_test_detail;Test test(__func__);fresh();
    SaveFile state=snapshot(7);test.assert(save(state),SaveStep::Done,"pre-wrap checkpoint");
    seedGeneration(8192,0xFFFF);
    SaveFile reload=snapshot();test.assert(storeBoot(reload),true,"0xFFFF checkpoint boots");
    test.assert(reload.playerLocation,7,"seeded 0xFFFF checkpoint selected");
    reload.playerLocation=0x1234;test.assert(save(reload),SaveStep::Done,"wrap save completes");
    SaveFile wrapped=snapshot();test.assert(storeBoot(wrapped),true,"post-wrap boot");
    test.assert(wrapped.playerLocation,0x1234,"0xFFFF -> 0 selects the wrapped newer checkpoint");
    wrapped.playerLocation=0x5678;test.assert(save(wrapped),SaveStep::Done,"post-wrap continuation save");
    SaveFile after=snapshot();test.assert(storeBoot(after),true,"post-wrap continuation boot");
    test.assert(after.playerLocation,0x5678,"generation continues past the wrap");
    suite.addTest(test);
}
// A recognized committed pre-v3 header (magic 0x43 0x47, version 1 or 2, commit
// bytes present) must reset fresh with a clean journal rather than import its
// arbitrary bytes. The existing corrupt-v3 case still fails closed.
inline void LegacyResetTest(TestSuite &suite) {
    using namespace save_test_detail;Test test(__func__);
    for(uint8_t version=1;version<=2;++version) {
        fresh();
        uint8_t legacy[16]={};
        legacy[0]=0x43;legacy[1]=0x47;legacy[2]=version;legacy[3]=16;
        for(uint8_t i=4;i<14;++i) legacy[i]=0x5a; // arbitrary non-v3 payload
        legacy[14]=0xa5;legacy[15]=0x5a;          // committed marker
        flashFakeSetBytes(8192,legacy,sizeof(legacy));
        SaveFile state=snapshot();
        test.assert(storeBoot(state),false,"pre-v3 committed header resets fresh");
        test.assert(storeBootFailed(),false,"pre-v3 reset is not a failure");
        test.assert(journalCount(),0,"pre-v3 reset leaves a blank journal");
        StoreRecord record={};
        test.assert(storeRead(0,record),true,"reset collection is readable");
        test.assert(record.id,255,"pre-v3 bytes are not imported as a creature");
        test.assert(save(state),SaveStep::Done,"fresh save works after pre-v3 reset");
    }
    suite.addTest(test);
}
// Bounded live-read proof: a checkpoint miss reads exactly one 8-byte record and
// a pending-overlay hit reads exactly one 16-byte journal record; repeated reads
// of an unchanged slot stay one bounded read each (no journal rescan). Deltas use
// the test backend's read counters; no window cache or runtime change.
inline void LiveReadBoundedTest(TestSuite &suite) {
    using namespace save_test_detail;Test test(__func__);fresh();
    SaveFile state=snapshot();StoreRecord record=creature();uint8_t slot=255;
    test.assert(storeAdd(record,slot),true,"seed instance");
    test.assert(save(state),SaveStep::Done,"seed checkpoint");
    StoreRecord out={};
    uint32_t calls=flashFakeReadCount(),bytes=flashFakeReadBytes();
    test.assert(storeRead(1,out),true,"checkpoint miss read");
    test.assert(flashFakeReadCount()-calls,1,"checkpoint miss performs one backend read");
    test.assert(flashFakeReadBytes()-bytes,8,"checkpoint miss reads one 8-byte record");
    StoreRecord pending=creature(41);
    test.assert(storeWrite(1,pending),true,"pending overlay write");
    calls=flashFakeReadCount();bytes=flashFakeReadBytes();
    test.assert(storeRead(1,out),true,"pending overlay hit read");
    test.assert(flashFakeReadCount()-calls,1,"pending hit performs one backend read");
    test.assert(flashFakeReadBytes()-bytes,16,"pending hit reads one 16-byte journal record");
    test.assert(out.exp,41,"pending hit returns overlay value");
    calls=flashFakeReadCount();bytes=flashFakeReadBytes();
    test.assert(storeRead(1,out),true,"repeated read one");
    test.assert(storeRead(1,out),true,"repeated read two");
    test.assert(flashFakeReadCount()-calls,2,"repeated reads issue one call each");
    test.assert(flashFakeReadBytes()-bytes,32,"repeated reads stay 16 bytes each without rescan");
    suite.addTest(test);
}
// Player/SaveFile v3 integration: byte-exact non-threshold XP, level-change
// threshold rewrite that leaves duplicates exact, slot/HP reassignment and
// empty-slot mapping, all through SaveController begin/advance/load.
inline void PlayerStoreIntegrationTest(TestSuite &suite) {
    using namespace save_test_detail;Test test(__func__);
    StoreRecord stored=creature(0);
    // Fractional XP (126 -> level 5; the next threshold is 216) stays byte-exact
    // while the RAM level still matches the stored exp-derived level.
    fresh();
    StoreRecord first=creature(126);uint8_t slotA=255;
    test.assert(storeAdd(first,slotA),true,"seed non-threshold instance");
    test.assert(storeAssign(0,slotA,17),true,"party references instance");
    player=Player();player.party[0].load(first);player.creatureHPs[0]=17;
    SaveController::begin(GameState_t::WORLD);
    for(uint16_t i=0;i<256 && saveInProgress();++i) SaveController::advance();
    test.assert(gameState.state==GameState_t::WORLD,true,"non-threshold save completes");
    player=Player();test.assert(SaveController::load(),true,"non-threshold checkpoint reloads");
    test.assert(storeRead(slotA,stored),true,"stored instance readable");
    test.assert(stored.exp,126,"non-threshold XP preserved when level unchanged");
    test.assert(stored.id,first.id,"party instance id preserved");
    test.assert(stored.attackMask,first.attackMask,"party instance mask preserved");
    // A real level change rewrites only that record to the new threshold; a
    // second duplicate instance keeps its exact fractional XP.
    fresh();
    StoreRecord member=creature(126);uint8_t slot0=255;
    test.assert(storeAdd(member,slot0),true,"seed party instance");
    StoreRecord dup=creature(130);uint8_t slot1=255;
    test.assert(storeAdd(dup,slot1),true,"seed duplicate instance");
    test.assert(storeAssign(0,slot0,17),true,"party references party instance");
    player=Player();player.party[0].load(member);player.creatureHPs[0]=17;
    SaveController::begin(GameState_t::WORLD);
    for(uint16_t i=0;i<256 && saveInProgress();++i) SaveController::advance();
    test.assert(gameState.state==GameState_t::WORLD,true,"baseline checkpoint save completes");
    player=Player();test.assert(SaveController::load(),true,"baseline checkpoint reloads");
    test.assert(storeRead(slot0,stored),true,"leveled instance readable");
    test.assert(stored.exp,126,"baseline fractional XP survives reload");
    player.party[0].level=6;
    SaveController::begin(GameState_t::WORLD);
    for(uint16_t i=0;i<256 && saveInProgress();++i) SaveController::advance();
    test.assert(gameState.state==GameState_t::WORLD,true,"level-change save completes");
    player=Player();test.assert(SaveController::load(),true,"level-change checkpoint reloads");
    test.assert(storeRead(slot0,stored),true,"leveled instance readable after reload");
    test.assert(stored.exp,levelExpThreshold(6),"level change rewrites only its threshold");
    test.assert(storeRead(slot1,stored),true,"duplicate readable");
    test.assert(stored.exp,130,"duplicate keeps its exact fractional XP");
    // Duplicate identity/slot, explicit HP reassignment and empty entries.
    fresh();
    StoreRecord a=creature(126);uint8_t s0=255;
    test.assert(storeAdd(a,s0),true,"seed first duplicate");
    StoreRecord b=creature(200);uint8_t s1=255;
    test.assert(storeAdd(b,s1),true,"seed second duplicate");
    test.assert(storeAssign(0,s0,17),true,"party references first");
    test.assert(storeAssign(0,s1,9),true,"explicit HP reassignment to second");
    player=Player();player.party[0].load(b);player.creatureHPs[0]=9;
    player.creatureHPs[2]=33; // deliberately nonzero live HP on an unassigned slot
    SaveController::begin(GameState_t::WORLD);
    for(uint16_t i=0;i<256 && saveInProgress();++i) SaveController::advance();
    SaveFile loaded={};test.assert(storeBoot(loaded),true,"reassignment checkpoint boots");
    test.assert(loaded.partySlots[0],s1,"reassigned party slot restored");
    test.assert(loaded.partyHP[0],9,"explicit reassignment HP restored");
    test.assert(loaded.partySlots[2],255,"empty party slot stays 255");
    test.assert(loaded.partyHP[2],0,"empty party slot stores canonical zero HP");
    player=Player();test.assert(SaveController::load(),true,"shipping load works");
    test.assert(storePartySlot(0),s1,"reassigned party slot restores after load");
    test.assert(player.party[0].id,b.id,"reassigned party identity restored");
    test.assert(player.creatureHPs[0],9,"reassigned party HP restores after load");
    test.assert(player.party[2].id,255,"empty entry restores id 255");
    test.assert(player.creatureHPs[2],0,"empty entry restores HP zero");
    suite.addTest(test);
}
// The one Player-owned bag is the v3 snapshot authority. Proves nonzero lure,
// consumable and key byte 0 round-trip while the version, 96-byte size and every
// wire offset stay unchanged, the key extension bytes stay zero, and an
// empty-reserve v3 snapshot still loads through the controller.
inline void PlayerBagPersistenceTest(TestSuite &suite) {
    using namespace save_test_detail;Test test(__func__);
    test.assert(static_cast<uint8_t>(SAVE_VERSION), static_cast<uint8_t>(3), "v3 version unchanged");
    test.assert(static_cast<uint8_t>(SAVE_SNAPSHOT_BYTES), static_cast<uint8_t>(96), "snapshot size unchanged");
    test.assert(sizeof(SaveFile), static_cast<size_t>(96), "SaveFile stays 96 bytes");
    test.assert(offsetof(SaveFile,lures), static_cast<size_t>(27), "lures wire offset");
    test.assert(offsetof(SaveFile,consumables), static_cast<size_t>(51), "consumables wire offset");
    test.assert(offsetof(SaveFile,keyItems), static_cast<size_t>(59), "keyItems wire offset");
    test.assert(offsetof(SaveFile,future), static_cast<size_t>(63), "future wire offset");

    fresh();
    player=Player();
    item::inventoryClear(player.inventory);
    item::keyItemsClear(player.keyItems);
    item::inventoryAdd(player.inventory,item::ItemKind::Lure,item::lureId(2,1),7);
    item::inventoryAdd(player.inventory,item::ItemKind::Lure,item::lureId(0,2),3);
    item::inventoryAdd(player.inventory,item::ItemKind::Consumable,4,9);
    item::keyItemUnlock(player.keyItems,5);
    SaveController::begin(GameState_t::WORLD);
    for(uint16_t i=0;i<256 && saveInProgress();++i) SaveController::advance();
    test.assert(gameState.state==GameState_t::WORLD,true,"bag save completes");
    SaveFile stored={};
    test.assert(saveFileLoad(stored),true,"bag snapshot loads");
    test.assert(stored.lures[item::lureId(2,1)],static_cast<uint8_t>(7),"lure count captured");
    test.assert(stored.lures[item::lureId(0,2)],static_cast<uint8_t>(3),"second lure captured");
    test.assert(stored.consumables[4],static_cast<uint8_t>(9),"consumable count captured");
    test.assert(stored.keyItems[0],static_cast<uint8_t>(1u<<5),"runtime key byte captured");
    for(uint8_t i=1;i<4;++i) test.assert(stored.keyItems[i],static_cast<uint8_t>(0),"key extension byte stays zero");
    for(uint8_t i=0;i<sizeof(stored.future);++i) test.assert(stored.future[i],static_cast<uint8_t>(0),"reserved tail stays zero");
    test.assert(saveFileValid(stored),true,"nonzero bag snapshot validates");

    // Wipe the live bag, reload, and prove the same one bag is restored.
    player=Player();
    test.assert(SaveController::load(),true,"bag checkpoint reloads");
    test.assert(item::inventoryCount(player.inventory,item::ItemKind::Lure,item::lureId(2,1)),static_cast<uint8_t>(7),"live lure restored");
    test.assert(item::inventoryCount(player.inventory,item::ItemKind::Consumable,4),static_cast<uint8_t>(9),"live consumable restored");
    test.assert(item::keyItemUnlocked(player.keyItems,5),true,"live key unlocked restored");
    test.assert(item::battleInventory()==&player.inventory,true,"battle pointer names the one bag");

    // Existing v3 saves with empty reserves still load.
    fresh();
    player=Player();
    item::inventoryClear(player.inventory);
    item::keyItemsClear(player.keyItems);
    SaveController::begin(GameState_t::WORLD);
    for(uint16_t i=0;i<256 && saveInProgress();++i) SaveController::advance();
    SaveFile empty={};test.assert(saveFileLoad(empty),true,"empty-reserve snapshot loads");
    for(uint8_t i=0;i<24;++i) test.assert(empty.lures[i],static_cast<uint8_t>(0),"empty lure reserve stays zero");
    for(uint8_t i=0;i<8;++i) test.assert(empty.consumables[i],static_cast<uint8_t>(0),"empty consumable reserve stays zero");
    test.assert(empty.keyItems[0],static_cast<uint8_t>(0),"empty key byte stays zero");
    test.assert(saveFileValid(empty),true,"empty-reserve v3 validates");
    item::inventoryAdd(player.inventory,item::ItemKind::Lure,item::lureId(1,0),4);
    test.assert(SaveController::load(),true,"empty-reserve v3 loads through controller");
    test.assert(item::inventoryCount(player.inventory,item::ItemKind::Lure,item::lureId(1,0)),static_cast<uint8_t>(0),"empty reserve restores zero");

    // Boundary validation stays on the existing offsets.
    SaveFile edge=snapshot();edge.lures[0]=item::COUNT_MAX;
    test.assert(saveFileValid(edge),true,"max count at a lure offset validates");
    edge.lures[0]=static_cast<uint8_t>(item::COUNT_MAX+1);
    test.assert(saveFileValid(edge),false,"count above max is refused");
    edge=snapshot();edge.keyItems[0]=0xff;
    test.assert(saveFileValid(edge),true,"runtime key byte 0 accepts any value");
    edge.keyItems[1]=0x01;
    test.assert(saveFileValid(edge),false,"key extension byte must stay zero");
    suite.addTest(test);
}
// Ordinary new-game start (CreatureGathererFX-jp8.11.24). The shipping sketch's
// !restored branch installs the one Player bag, applies the authored "beta"
// preset (species 0/3/6, level 3, full HP, authored moves) and then seeds the v3
// collection. This mirrors that boot order with the same generated preset and
// load seams the firmware uses, and proves a valid restore keeps its own party
// instead of being overwritten by a fresh beta start.
inline void FreshBetaStartTest(TestSuite &suite) {
    using namespace save_test_detail;Test test(__func__);
    const uint8_t betaSpecies[3]={0,3,6};
    const uint8_t betaMasks[3]={2,4,2};
    const uint16_t betaLocation=static_cast<uint16_t>((5u<<8)|11u); // (11,5)

    fresh();
    player=Player();
    battle::applyPlayerPreset(player,BattlePresets::beta);
    item::setBattleInventory(&player.inventory);
    SaveController::seedParty();
    gameState.playerLocation=betaLocation;
    for(uint8_t i=0;i<3;++i) {
        const uint8_t maxHP=player.party[i].hp;
        test.assert(player.party[i].id,betaSpecies[i],"fresh beta species");
        test.assert(player.party[i].level,3,"fresh beta level");
        test.assert(player.party[i].attackMask,betaMasks[i],"fresh beta authored moves");
        test.assert(player.creatureHPs[i],maxHP,"fresh beta starts at full HP");
        StoreRecord record={};
        test.assert(storeRead(i,record),true,"fresh beta stable instance");
        test.assert(record.id,betaSpecies[i],"fresh beta instance id");
        test.assert(record.exp,levelExpThreshold(3),"fresh beta instance level3 experience");
        test.assert(record.hpEncoded,static_cast<uint8_t>(maxHP+1),"fresh beta encoded full HP");
        test.assert(record.attackMask,betaMasks[i],"fresh beta instance moves");
        test.assert(storePartySlot(i),i,"fresh beta party names its stable instance");
        test.assert(storePartyHP(i),maxHP,"fresh beta party HP assignment");
    }
    test.assert(item::battleInventory()==&player.inventory,true,"fresh beta bag is the battle inventory");
    test.assert(item::inventoryNonZeroCount(player.inventory),0,"fresh beta bag starts empty");

    // Explicit-save round trip of the fresh party and its authored location.
    SaveController::begin(GameState_t::WORLD);
    for(uint16_t i=0;i<256 && saveInProgress();++i) SaveController::advance();
    player=Player();
    test.assert(SaveController::load(),true,"fresh beta checkpoint restores");
    test.assert(gameState.playerLocation,betaLocation,"fresh beta location round-trips");
    for(uint8_t i=0;i<3;++i) {
        test.assert(player.party[i].id,betaSpecies[i],"restored beta species");
        test.assert(player.party[i].level,3,"restored beta level");
    }

    // A valid restore keeps its own party. The legacy species-only default
    // (species 0/1/2 at the maximum level) stands in for any different saved
    // party: booting it must not reapply the beta start.
    fresh();player=Player();player.basic();SaveController::seedParty();
    gameState.playerLocation=0x4321;
    SaveController::begin(GameState_t::WORLD);
    for(uint16_t i=0;i<256 && saveInProgress();++i) SaveController::advance();
    player=Player();
    const bool restored=SaveController::load();
    test.assert(restored,true,"alternate party save restores");
    test.assert(gameState.playerLocation,0x4321,"alternate location retained");
    for(uint8_t i=0;i<3;++i)
        test.assert(player.party[i].id,i,"restored party is not overwritten by the beta start");
    test.assert(player.party[0].level,31,"restored level is not rewritten by the beta start");
    suite.addTest(test);
}
// jp8.11.12: a boxed (non-party) stable instance whose accepted stored
// experience has already reached its authored edge evolves in place on the
// collection transition. exp, slot and attack mask (moves) are preserved; the
// species id becomes the explicit target and HP is re-encoded for the target at
// the same derived level, keeping a living instance's missing HP and leaving a KO
// instance KO. A refused write leaves the old species, so the pending evolution
// stays derivable from exp and retryable after the journal drains.
inline void BoxedEvolutionTest(TestSuite &suite) {
    using namespace save_test_detail;Test test(__func__);
    // Host fixture for the packed evolution mapping: the three parallel columns
    // (source, target, level) are contiguous, so offsets come from published
    // addresses. Anchor the fake at the mapping base; the zeroed reward region
    // resolves to the no-material sentinel pair (item 0, count 0).
    const uint32_t targetOffset = static_cast<uint32_t>(Evolution::evolutionTarget - Evolution::evolutionSource);
    const uint32_t levelOffset = static_cast<uint32_t>(Evolution::evolutionLevel - Evolution::evolutionSource);
    fxDataFake::dataBase = Evolution::evolutionSource;
    memset(fxDataFake::dataBytes, 0, sizeof(fxDataFake::dataBytes));
    fxDataFake::dataBytes[0]=0;fxDataFake::dataBytes[1]=3;fxDataFake::dataBytes[2]=6;
    fxDataFake::dataBytes[targetOffset+0]=1;fxDataFake::dataBytes[targetOffset+1]=4;fxDataFake::dataBytes[targetOffset+2]=7;
    fxDataFake::dataBytes[levelOffset+0]=8;fxDataFake::dataBytes[levelOffset+1]=8;fxDataFake::dataBytes[levelOffset+2]=8;

    // A full edge instance evolves to its explicit target; non-edge species and
    // out-of-range slots are refused; the evolved instance never re-evolves.
    fresh();
    StoreRecord edge=creature(levelExpThreshold(8));edge.id=0;edge.attackMask=2;edge.hpEncoded=0;
    StoreRecord plain=creature(levelExpThreshold(8));plain.id=2;plain.attackMask=2;plain.hpEncoded=0;
    uint8_t edgeSlot=255,plainSlot=255;
    test.assert(storeAdd(edge,edgeSlot),true,"add the edge instance");
    test.assert(storeAdd(plain,plainSlot),true,"add the non-edge instance");
    test.assert(storeEvolve(edgeSlot),true,"the qualifying boxed instance evolves");
    StoreRecord evolved={};test.assert(storeRead(edgeSlot,evolved),true,"the evolved instance is readable");
    test.assert(evolved.id,1,"species 0 becomes its explicit target 1");
    test.assert(evolved.exp,levelExpThreshold(8),"exp is preserved");
    test.assert(evolved.attackMask,2,"the attack mask (moves) is preserved");
    test.assert(evolved.hpEncoded,0,"a full instance stays full after evolution");
    test.assert(storeEvolve(edgeSlot),false,"an evolved instance never re-evolves");
    test.assert(storeEvolve(plainSlot),false,"a species without an authored edge never evolves");
    StoreRecord held={};test.assert(storeRead(plainSlot,held),true,"the non-edge instance is readable");
    test.assert(held.id,2,"the non-edge species is unchanged");
    test.assert(storeEvolve(200),false,"an out-of-range slot is refused");

    // Duplicate identities stay separate: two copies of the same species evolve
    // independently to the same target without aliasing each other's slot.
    StoreRecord dupA=creature(levelExpThreshold(8));dupA.id=3;dupA.attackMask=2;dupA.hpEncoded=0;
    StoreRecord dupB=dupA;
    uint8_t dupSlotA=255,dupSlotB=255;
    test.assert(storeAdd(dupA,dupSlotA),true,"add the first duplicate edge instance");
    test.assert(storeAdd(dupB,dupSlotB),true,"add the second duplicate edge instance");
    test.assert(dupSlotA!=dupSlotB,true,"duplicates occupy distinct slots");
    test.assert(storeEvolve(dupSlotA),true,"the first duplicate evolves");
    test.assert(storeEvolve(dupSlotB),true,"the second duplicate evolves");
    StoreRecord dupRead={};test.assert(storeRead(dupSlotA,dupRead),true,"the first duplicate is readable");
    test.assert(dupRead.id,4,"the first duplicate became its target");
    test.assert(storeRead(dupSlotB,dupRead),true,"the second duplicate is readable");
    test.assert(dupRead.id,4,"the second duplicate became its target");

    // Below the edge: no evolution.
    fresh();
    StoreRecord low=creature(static_cast<uint16_t>(levelExpThreshold(8)-1));low.id=0;low.attackMask=2;low.hpEncoded=0;
    uint8_t lowSlot=255;
    test.assert(storeAdd(low,lowSlot),true,"add a below-edge instance");
    test.assert(storeEvolve(lowSlot),false,"a below-edge instance does not evolve");
    held={};test.assert(storeRead(lowSlot,held),true,"the below-edge instance is readable");
    test.assert(held.id,0,"the below-edge species is unchanged");

    // Missing HP is preserved for a living instance; a KO instance stays KO.
    fresh();
    StoreRecord injured=creature(levelExpThreshold(8));injured.id=6;injured.attackMask=2;
    Creature c6;c6.load(injured);const uint8_t max6=c6.hp;
    injured.hpEncoded=static_cast<uint8_t>(max6-5+1); // five HP missing
    uint8_t injuredSlot=255;
    test.assert(storeAdd(injured,injuredSlot),true,"add an injured edge instance");
    test.assert(storeEvolve(injuredSlot),true,"the injured instance evolves");
    StoreRecord evInj={};test.assert(storeRead(injuredSlot,evInj),true,"the injured evolved instance is readable");
    test.assert(evInj.id,7,"species 6 becomes 7");
    Creature c7;c7.load(evInj);const uint8_t max7=c7.hp;
    test.assert(storeInstanceHP(evInj),static_cast<uint8_t>(max7-5),"missing HP is preserved across evolution");

    StoreRecord ko=creature(levelExpThreshold(8));ko.id=0;ko.attackMask=2;ko.hpEncoded=1; // KO
    uint8_t koSlot=255;
    test.assert(storeAdd(ko,koSlot),true,"add a KO edge instance");
    test.assert(storeEvolve(koSlot),true,"a KO edge instance still evolves");
    StoreRecord evKo={};test.assert(storeRead(koSlot,evKo),true,"the KO evolved instance is readable");
    test.assert(evKo.id,1,"the KO instance changes species");
    test.assert(storeInstanceHP(evKo),0,"a KO instance stays KO after evolution");

    // Refusal/retry: a saturated pending index refuses a fresh evolution write
    // (the record is committed, not pending); the old species is retained and the
    // pending evolution retries once the journal drains into a checkpoint.
    fresh();
    StoreRecord retry=creature(levelExpThreshold(8));retry.id=0;retry.attackMask=2;retry.hpEncoded=0;
    uint8_t retrySlot=255;
    test.assert(storeAdd(retry,retrySlot),true,"seed the retry instance");
    SaveFile base=snapshot();test.assert(save(base),SaveStep::Done,"commit the retry instance");
    StoreRecord filler=creature(27);
    for(uint8_t n=0;n<16;++n){uint8_t s=255;test.assert(storeAdd(filler,s),true,"fill the pending index");}
    test.assert(storeEvolve(retrySlot),false,"a saturated journal refuses the evolution");
    held={};test.assert(storeRead(retrySlot,held),true,"the refused instance is readable");
    test.assert(held.id,0,"a refused evolution keeps the old species");
    SaveFile drain=snapshot();test.assert(save(drain),SaveStep::Done,"the journal drains into a checkpoint");
    test.assert(storeEvolve(retrySlot),true,"the pending evolution retries after the drain");
    StoreRecord applied={};test.assert(storeRead(retrySlot,applied),true,"the retried instance is readable");
    test.assert(applied.id,1,"the retried evolution applies the target");
    suite.addTest(test);
}

inline void SaveSuite(TestRunner &runner) {
    TestSuite suite("Save Suite");CheckpointCollectionTest(suite);CollectionCapacityTest(suite);
    BoxedInstanceHPTest(suite);BoxedHPPublicationCutsTest(suite);SaveControllerCaptureHPCutsTest(suite);
    CheckpointCutTest(suite);JournalDecodeTest(suite);JournalPublicationTest(suite);ShippingSaveRestoreTest(suite);
    SaveFlagExtentTest(suite);GenerationWrapTest(suite);LegacyResetTest(suite);LiveReadBoundedTest(suite);
    PlayerStoreIntegrationTest(suite);PlayerBagPersistenceTest(suite);FreshBetaStartTest(suite);
    BoxedEvolutionTest(suite);
    runner.addTestSuite(suite);
}
