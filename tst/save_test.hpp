#pragma once
#include <string.h>
#include "test.hpp"
#include "../src/save/Compaction.hpp"
#include "../src/save/FlashBackend.hpp"
#include "../src/save/Journal.hpp"
#include "../src/save/SaveController.hpp"
#include "../src/creature/Creature.hpp"
#include "../src/creature/LevelCurve.hpp"
#include "../src/flags/flag_bit_array.hpp"
#include "../src/globals.hpp"
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
inline StoreRecord creature(uint16_t exp=27) { return {0,exp,{8,32,32,32},0}; }
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
    const uint8_t emptyMoves[4]={0,0,0,0};
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
            test.assert(memcmp(got.moves,added?record.moves:emptyMoves,4),0,"cut record moves");
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
inline void SaveSuite(TestRunner &runner) {
    TestSuite suite("Save Suite");CheckpointCollectionTest(suite);CollectionCapacityTest(suite);
    CheckpointCutTest(suite);JournalDecodeTest(suite);JournalPublicationTest(suite);ShippingSaveRestoreTest(suite);
    SaveFlagExtentTest(suite);GenerationWrapTest(suite);LegacyResetTest(suite);LiveReadBoundedTest(suite);
    runner.addTestSuite(suite);
}
