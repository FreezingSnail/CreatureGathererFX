#pragma once

#include <stdint.h>

#include "SaveFile.hpp"

enum class SaveStep : uint8_t {
    Idle,
    Replay,
    Commit,
    Verify,
    EraseJournal,
    Done,
    Failed,
};

void saveBegin();
bool saveInProgress();
SaveStep saveStepAdvance(SaveFile &state);
