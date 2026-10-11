#pragma once
#include "SaveFile.hpp"
#include "StoreRecord.hpp"
enum class SaveStep : uint8_t {
    Idle, PrepareSpare, VerifySpare, CopySnapshot, CopyStore, StoreHeader,
    Commit, Verify, EraseJournal, VerifyJournalErase, ClearCommit, ClearVerify, Done, Failed
};
bool storeBoot(SaveFile &state);
bool storeBootFailed();
void storeFresh();
bool storeRead(uint8_t slot, StoreRecord &out);
bool storeWrite(uint8_t slot, const StoreRecord &in);
bool storeAdd(const StoreRecord &in, uint8_t &slot);
bool storeRemove(uint8_t slot);
bool storeAssign(uint8_t partyIndex, uint8_t slot, uint8_t hp);
uint8_t storePartyHP(uint8_t partyIndex);
void storeSetPartyHP(uint8_t partyIndex, uint8_t hp);
uint8_t storePartySlot(uint8_t partyIndex);
void saveBegin();
bool saveInProgress();
SaveStep saveStepAdvance(SaveFile &state);
