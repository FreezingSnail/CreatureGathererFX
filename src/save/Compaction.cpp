#include "Compaction.hpp"
#include <string.h>
#include "FlashBackend.hpp"
#include "Journal.hpp"
#include "../lib/ReadData.hpp"
#include "../creature/LevelCurve.hpp"

namespace {
constexpr uint16_t STORE_A = 8192, STORE_B = 12288;
struct Pending { uint8_t slot, journal; };
Pending pending[16];
uint8_t pendingCount = 0;
uint8_t partySlots[3] = {255,255,255};
uint8_t partyHP[3] = {};
uint8_t scratch[64];
uint16_t active = 0, destination = STORE_A, generation = 0, offset = 0;
uint8_t sum1 = 0, sum2 = 0, liveCount = 0;
bool failedBoot = false;
SaveStep step = SaveStep::Idle;

uint16_t word(const uint8_t *p) { return static_cast<uint16_t>(p[0]) | (static_cast<uint16_t>(p[1]) << 8); }
void putWord(uint8_t *p, uint16_t value) { p[0] = value; p[1] = value >> 8; }
__attribute__((noinline)) void checksum(uint8_t &a, uint8_t &b, const uint8_t *bytes, uint8_t count) {
    for (uint8_t i=0; i<count; ++i) {
        uint16_t n = static_cast<uint16_t>(a) + bytes[i];
        a = n >= 255 ? n - 255 : n;
        n = static_cast<uint16_t>(b) + a;
        b = n >= 255 ? n - 255 : n;
    }
}
__attribute__((noinline)) bool blank(uint16_t base, uint16_t at) {
    flash.readBytes(base + at, scratch, 64);
    for (uint8_t i=0;i<64;++i) if (scratch[i]!=255) return false;
    return true;
}
void empty(StoreRecord &out) { memset(&out,0,sizeof(out)); out.id=255; }
bool recordValid(const uint8_t *bytes) {
    if (bytes[7]) return false;
    if (bytes[0] == 255) for (uint8_t i=1;i<8;++i) if (bytes[i]) return false;
    return true;
}
__attribute__((noinline)) bool validate(uint16_t base, uint16_t &gen, uint8_t &cleanup) {
    flash.readBytes(base,scratch,16);
    if (scratch[0]!=0x43 || scratch[1]!=0x47 || scratch[2]!=3 || scratch[3]!=16 ||
        scratch[6]!=64 || scratch[7]!=96 || scratch[8]>64 || scratch[9]>1 ||
        scratch[14]!=0xa5 || scratch[15]!=0x5a) return false;
    gen=word(scratch+4); cleanup=scratch[9];
    const uint8_t expectedCount=scratch[8];
    const uint16_t expectedBody=word(scratch+10), expectedHeader=word(scratch+12);
    scratch[9]=1;
    uint8_t a=0,b=0; checksum(a,b,scratch,12);
    if ((static_cast<uint16_t>(b)<<8 | a)!=expectedHeader) return false;
    a=0;b=0;
    for (uint16_t at=16;at<624;at+=64) {
        uint8_t n = 624-at < 64 ? 624-at : 64;
        flash.readBytes(base+at,scratch,n); checksum(a,b,scratch,n);
    }
    if ((static_cast<uint16_t>(b)<<8 | a)!=expectedBody) return false;
    flash.readBytes(base+16,scratch,64);
    uint8_t slots[3]; memcpy(slots,scratch+10,3);
    if(!savePartySlotsValid(slots)) return false;
    for(uint8_t i=2;i<64;++i) if(!saveSnapshotByteValid(i,scratch[i])) return false;
    flash.readBytes(base+80,scratch,32);
    for(uint8_t i=0;i<32;++i) if(!saveSnapshotByteValid(i+64,scratch[i])) return false;
    uint8_t count=0;
    for(uint8_t slot=0;slot<64;++slot) {
        flash.readBytes(storeRecordAddr(base,slot),scratch,8);
        if(!recordValid(scratch)) return false;
        if(scratch[0]!=255) ++count;
        else for(uint8_t i=0;i<3;++i) if(slots[i]==slot) return false;
    }
    return count==expectedCount;
}
bool pendingEntry(uint8_t slot, uint8_t &entry) {
    for(entry=0;entry<pendingCount;++entry) if(pending[entry].slot==slot) return true;
    return false;
}
uint16_t replayIndex=0;
uint8_t maxHP(const StoreRecord &record) {
    const uint8_t level=levelFromExp(record.exp);
    const CreatureData_t seed=getCreatureFromStore(record.id);
    const uint8_t stat=2*level+seed.hpSeed*(level/3);
    return stat+30;
}
bool assignmentValid(uint8_t index,uint16_t slot,uint8_t hp) {
    if(index>=3 || (slot>=64 && slot!=65535)) return false;
    if(slot==65535) return hp==0;
    for(uint8_t i=0;i<3;++i) if(i!=index && partySlots[i]==slot) return false;
    StoreRecord record;
    return storeRead(slot,record) && record.id!=255 && hp<=maxHP(record);
}
void replay(const JournalRecord &record) {
    if (record.op == static_cast<uint8_t>(LogOp::PartyAssign)) {
        if(assignmentValid(record.payload[0],record.slot,record.payload[1])) {
            partySlots[record.payload[0]]=record.slot;
            partyHP[record.payload[0]]=record.payload[1];
        }
    } else if(record.slot<64 && (record.op==2 || record.op==3)) {
        bool valid = record.op==3 || (record.payload[0]!=255 && recordValid(record.payload));
        if(record.op==3) for(uint8_t i=0;i<3;++i) if(partySlots[i]==record.slot) valid=false;
        uint8_t entry;
        if(valid && !pendingEntry(record.slot,entry) && pendingCount==16) {
            failedBoot=true;
        } else if(valid) {
            if(entry==pendingCount) ++pendingCount;
            pending[entry]={static_cast<uint8_t>(record.slot),static_cast<uint8_t>(replayIndex)};
        }
    }
    ++replayIndex;
}
bool append(LogOp op,uint16_t slot,const StoreRecord *record,uint8_t partyIndex=0,uint8_t hp=0) {
    if(saveInProgress() || failedBoot) return false;
    JournalRecord log={}; log.op=static_cast<uint8_t>(op); log.slot=slot;
    if(record) storeRecordEncode(*record,log.payload);
    if(op==LogOp::PartyAssign) { log.payload[0]=partyIndex;log.payload[1]=hp; }
    const uint16_t index=journalLength();
    if(!journalAppend(log)) return false;
    replayIndex=index; replay(log);
    return true;
}
}

bool storeBootFailed() { return failedBoot; }
void storeFresh() {
    active=0;generation=0;pendingCount=0;failedBoot=false;step=SaveStep::Idle;
    memset(partySlots,255,3);memset(partyHP,0,3);journalErase();while(flash.busy()) {}
    for(uint16_t at=0;at<4096;at+=64) if(!blank(4096,at)) {failedBoot=true;break;}
}
bool storeBoot(SaveFile &state) {
    step=SaveStep::Idle;pendingCount=0;failedBoot=false;
    uint16_t ga=0,gb=0;uint8_t ca=0,cb=0;
    const bool va=validate(STORE_A,ga,ca),vb=validate(STORE_B,gb,cb);
    active=va ? STORE_A : 0;generation=ga;uint8_t cleanup=ca;
    if(vb && (!va || static_cast<int16_t>(gb-ga)>0)) {active=STORE_B;generation=gb;cleanup=cb;}
    if(!active) {
        for(uint8_t sector=0;sector<2;++sector) {
            flash.readBytes(sector ? STORE_B : STORE_A,scratch,16);
            bool erased=true;for(uint8_t i=0;i<16;++i) if(scratch[i]!=255) erased=false;
            if(!erased && scratch[14]==0xa5 && scratch[15]==0x5a &&
               !(scratch[0]==0x43 && scratch[1]==0x47 && scratch[2]<3)) failedBoot=true;
        }
        if(!failedBoot) storeFresh();
        return false;
    }
    if(cleanup) {
        journalErase();while(flash.busy()) {}
        for(uint16_t at=0;at<4096;at+=64) if(!blank(4096,at)) {failedBoot=true;return false;}
        const uint8_t zero=0;flash.writeBytes(active+9,&zero,1);
        flash.readBytes(active+9,scratch,1);if(scratch[0]) {failedBoot=true;return false;}
    }
    // Decode the snapshot in two bounded pieces, without a second RAM snapshot.
    flash.readBytes(active+16,scratch,64);
    state.playerLocation=word(scratch);memcpy(state.flags,scratch+2,62);
    flash.readBytes(active+80,scratch,32);memcpy(state.future+1,scratch,32);
    memcpy(partySlots,state.partySlots,3);memcpy(partyHP,state.partyHP,3);
    replayIndex=0;journalReplay(replay);
    memcpy(state.partySlots,partySlots,3);memcpy(state.partyHP,partyHP,3);
    return !failedBoot;
}
bool storeRead(uint8_t slot, StoreRecord &out) {
    if(slot>=64) return false;
    uint8_t entry,bytes[16];
    if(pendingEntry(slot,entry)) {
        flash.readBytes(4096+static_cast<uint16_t>(pending[entry].journal)*16,bytes,16);
        if(bytes[1]==3) empty(out);else storeRecordDecode(bytes+4,out);
    } else if(active) {
        flash.readBytes(storeRecordAddr(active,slot),bytes,8);storeRecordDecode(bytes,out);
    } else empty(out);
    return true;
}
bool storeWrite(uint8_t slot,const StoreRecord &in) {
    uint8_t entry;
    if(slot>=64 || in.id==255 || (!pendingEntry(slot,entry) && pendingCount==16)) return false;
    return append(LogOp::StoreAdd,slot,&in);
}
bool storeAdd(const StoreRecord &in,uint8_t &slot) {
    if(in.id==255) return false;
    StoreRecord current;
    for(uint8_t i=0;i<64;++i) {
        storeRead(i,current);
        if(current.id==255 && storeWrite(i,in)) {slot=i;return true;}
        if(current.id==255) return false;
    }
    return false;
}
bool storeRemove(uint8_t slot) {
    uint8_t entry;
    if(slot>=64 || (!pendingEntry(slot,entry) && pendingCount==16)) return false;
    for(uint8_t i=0;i<3;++i) if(partySlots[i]==slot) return false;
    return append(LogOp::StoreRemove,slot,nullptr);
}
bool storeAssign(uint8_t index,uint8_t slot,uint8_t hp) {
    const uint16_t ref=slot==255 ? 65535 : slot;
    if(!assignmentValid(index,ref,hp)) return false;
    return append(LogOp::PartyAssign,ref,nullptr,index,hp);
}
uint8_t storePartyHP(uint8_t index) { return index<3 ? partyHP[index] : 0; }
void storeSetPartyHP(uint8_t index,uint8_t hp) { if(index<3) partyHP[index]=hp; }
uint8_t storePartySlot(uint8_t index) { return index<3 ? partySlots[index] : 255; }
void saveBegin() {
    destination=active==STORE_A ? STORE_B : STORE_A;
    offset=0;sum1=0;sum2=0;liveCount=0;
    step=failedBoot ? SaveStep::Failed : SaveStep::PrepareSpare;
}
bool saveInProgress() { return step!=SaveStep::Idle && step!=SaveStep::Done && step!=SaveStep::Failed; }
SaveStep saveStepAdvance(SaveFile &state) {
    if(flash.busy()) return step;
    switch(step) {
    case SaveStep::PrepareSpare:
        memcpy(state.partySlots,partySlots,3);memcpy(state.partyHP,partyHP,3);
        if(!saveFileValid(state)) {step=SaveStep::Failed;break;}
        flash.eraseSector(destination>>8);offset=0;step=SaveStep::VerifySpare;break;
    case SaveStep::VerifySpare:
        if(!blank(destination,offset)) {step=SaveStep::Failed;break;}
        offset+=64;if(offset==4096) {offset=0;step=SaveStep::CopySnapshot;}break;
    case SaveStep::CopySnapshot: {
        const uint8_t n=offset==0 ? 64 : 32;
        saveFileEncode(state,offset,scratch,n);checksum(sum1,sum2,scratch,n);
        flash.writeBytes(destination+16+offset,scratch,n);offset+=n;
        if(offset==96) {offset=0;step=SaveStep::CopyStore;}break;
    }
    case SaveStep::CopyStore: {
        StoreRecord record;
        for(uint8_t i=0;i<8;++i) {
            storeRead(offset+i,record);if(record.id!=255) ++liveCount;
            storeRecordEncode(record,scratch+i*8);
        }
        checksum(sum1,sum2,scratch,64);
        flash.writeBytes(storeRecordAddr(destination,offset),scratch,64);
        offset+=8;if(offset==64) step=SaveStep::StoreHeader;break;
    }
    case SaveStep::StoreHeader: {
        scratch[0]=0x43;scratch[1]=0x47;scratch[2]=3;scratch[3]=16;
        putWord(scratch+4,generation+1);scratch[6]=64;scratch[7]=96;scratch[8]=liveCount;scratch[9]=1;
        scratch[10]=sum1;scratch[11]=sum2;
        uint8_t a=0,b=0;checksum(a,b,scratch,12);scratch[12]=a;scratch[13]=b;
        flash.writeBytes(destination,scratch,14);step=SaveStep::Commit;break;
    }
    case SaveStep::Commit:
        scratch[0]=0xa5;scratch[1]=0x5a;flash.writeBytes(destination+14,scratch,2);step=SaveStep::Verify;break;
    case SaveStep::Verify: {
        uint16_t gen;uint8_t cleanup;
        if(!validate(destination,gen,cleanup)) {step=SaveStep::Failed;break;}
        active=destination;generation=gen;pendingCount=0;step=SaveStep::EraseJournal;break;
    }
    case SaveStep::EraseJournal:
        journalErase();offset=0;step=SaveStep::VerifyJournalErase;break;
    case SaveStep::VerifyJournalErase:
        if(!blank(4096,offset)) {step=SaveStep::Failed;break;}
        offset+=64;if(offset==4096) step=SaveStep::ClearCommit;break;
    case SaveStep::ClearCommit:
        scratch[0]=0;flash.writeBytes(active+9,scratch,1);step=SaveStep::ClearVerify;break;
    case SaveStep::ClearVerify:
        flash.readBytes(active+9,scratch,1);step=scratch[0]==0 ? SaveStep::Done : SaveStep::Failed;break;
    default:break;
    }
    return step;
}
