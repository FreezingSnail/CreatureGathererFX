#pragma once
#include <stdint.h>

enum class BattleEventType {
    NONE,
    ATTACK,
    OPPONENT_ATTACK,
    DAMAGE,
    OPPONENT_DAMAGE,
    HEAL,
    OPPONENT_HEAL,
    STATUS,
    OPPONENT_STATUS,
    STAT,
    EVOLVE,
    LEVELUP,
    FAINT,
    OPPONENT_FAINT,
    WIN,
    LOSE,
    RUN,
    CATCH,
    ESCAPE,
    ITEM,
    SWITCH,
    FLEE,
    WAIT,
};

struct BattleEvent {
    BattleEventType type;
    // used for name index, committer, move, etc
    uint8_t index;
    // used for numerical data like damage
    uint8_t data;
};