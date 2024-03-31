#pragma once
#include <stdint.h>

// This is an encounter action
// attack, change, item, escape
enum ActionType { ATTACK = 0, CHNGE = 1, GATHER = 2, ESCAPE = 3, SKIP = 4 };
enum Priority { SLOW = 0, NORMAL = 1, FAST = 2 };

class Action {
  public:
    Priority priority;
    ActionType actionType;
    int8_t actionIndex;

    void setActionType(ActionType type, Priority priority);
};