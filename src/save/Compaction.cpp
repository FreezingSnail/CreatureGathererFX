#include "Compaction.hpp"

#include "FlashBackend.hpp"
#include "Journal.hpp"

namespace {
SaveStep saveStep = SaveStep::Idle;
bool eraseIssued = false;

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
        // Until store compaction lands, the live SaveFile snapshot owns the
        // party. Store ops cannot be replayed into Creature bytes.
        saveStep = SaveStep::Commit;
        break;
    }

    case SaveStep::Commit:
        state.version = SAVE_VERSION;
        state.checksum = saveFileChecksum(state);
        saveFileCommitPrepared(state);
        saveStep = SaveStep::Verify;
        break;

    case SaveStep::Verify: {
        if (!saveFileMatchesStored(state)) {
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
