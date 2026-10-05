#pragma once

#include "BattleTypes.hpp"
#include <stdint.h>

namespace battle {

struct ActiveView {
    uint8_t id;
    uint8_t hp;
    uint8_t maxHp;
};

struct PartySummary {
    uint8_t id;
    uint8_t hp;
    uint8_t alive;
};

struct BattleView {
    ActiveView active[2];
    PartySummary party[2][3];
    uint8_t partyCount[2];
    uint8_t activeSlot[2];
    uint8_t moveIds[4];
    uint8_t remainingUses[4] = {255, 255, 255, 255};
    uint8_t useLimitsPacked = 0;
    uint8_t gatherProgress;
    uint8_t gatherNeed;
};

struct MoveSnapshot {
    uint8_t moveIds[4];
    uint8_t remainingUses[4] = {255, 255, 255, 255};
    uint8_t useLimitsPacked = 0;
};

struct PartyChoice {
    uint8_t id;
    uint8_t slot;
    uint8_t hp;
};

struct PartySnapshot {
    PartyChoice choices[2];
    uint8_t count;
    uint8_t forced;
};

} // namespace battle
