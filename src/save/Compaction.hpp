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
uint8_t storeInstanceHP(const StoreRecord &record);
bool storeWrite(uint8_t slot, const StoreRecord &in);
// jp8.11.12: evolve one stable instance in place on the collection
// open/assignment transition when its accepted stored experience already
// qualifies for its authored edge. exp, slot and attack mask (moves) are
// preserved; the species id is rewritten to the explicit target and the HP byte
// re-encoded for the target at the same derived level, keeping a living
// instance's missing HP and leaving a KO instance KO. Returns true only when the
// species changed and the write was accepted; a refusal leaves the record
// untouched, so the pending evolution stays derivable from exp and retryable.
bool storeEvolve(uint8_t slot);
bool storeAdd(const StoreRecord &in, uint8_t &slot);
bool storeRemove(uint8_t slot);
bool storeAssign(uint8_t partyIndex, uint8_t slot, uint8_t hp);
uint8_t storePartyHP(uint8_t partyIndex);
void storeSetPartyHP(uint8_t partyIndex, uint8_t hp);
uint8_t storePartySlot(uint8_t partyIndex);
void saveBegin();
bool saveInProgress();
SaveStep saveStepAdvance(SaveFile &state);
