#include "Compaction.hpp"

#include <string.h>

#include "FlashBackend.hpp"
#include "Journal.hpp"

namespace {
SaveStep saveStep = SaveStep::Idle;
SaveFile *replayState = nullptr;
bool eraseIssued = false;

void applyJournalRecord(const JournalRecord &record)
{
    if (record.slot >= PARTY_MAX) {
        return;
    }

    Creature &creature = replayState->party[record.slot];
    switch (record.op) {
    case 0: // add
        memcpy(&creature, record.payload, sizeof(record.payload));
        break;
    case 1: // remove
        memset(&creature, 0, sizeof(creature));
        break;
    default:
        break;
    }
}

bool saveFileMatches(const SaveFile &expected, const SaveFile &actual)
{
    return actual.checksum == saveFileChecksum(actual) &&
           memcmp(&expected, &actual, sizeof(SaveFile)) == 0;
}
} // namespace

void saveBegin()
{
    saveStep = SaveStep::Replay;
    eraseIssued = false;
}

bool saveInProgress()
{
    return saveStep == SaveStep::Replay || saveStep == SaveStep::Commit ||
           saveStep == SaveStep::Verify || saveStep == SaveStep::EraseJournal;
}

SaveStep saveStepAdvance(SaveFile &state)
{
    if (flash.busy()) {
        return saveStep;
    }

    switch (saveStep) {
    case SaveStep::Replay: {
        SaveFile committed = {};
        // Callers pass a fresh snapshot of live RAM. Restore only the
        // journal-owned party baseline, then apply its slot mutations. This
        // avoids replacing unrelated live fields (location, flags, plants, and
        // inventory); without a valid commit, the supplied new-game state stays
        // intact and receives any journal records.
        if (saveFileLoad(committed)) {
            memcpy(state.party, committed.party, sizeof(state.party));
        }

        replayState = &state;
        journalReplay(applyJournalRecord);
        replayState = nullptr;
        saveStep = SaveStep::Commit;
        break;
    }

    case SaveStep::Commit:
        state.version = SAVE_VERSION;
        state.checksum = saveFileChecksum(state);
        saveFileCommit(state);
        saveStep = SaveStep::Verify;
        break;

    case SaveStep::Verify: {
        SaveFile persisted = {};
        if (!saveFileLoad(persisted) || !saveFileMatches(state, persisted)) {
            saveStep = SaveStep::Failed;
            break;
        }
        saveStep = SaveStep::EraseJournal;
        break;
    }

    case SaveStep::EraseJournal:
        if (!eraseIssued) {
            journalErase();
            eraseIssued = true;
            break;
        }
        saveStep = SaveStep::Done;
        break;

    case SaveStep::Idle:
    case SaveStep::Done:
    case SaveStep::Failed:
        break;
    }

    return saveStep;
}
