#pragma once
#include <stdint.h>

// This is an encounter action
// attack, change, item, escape
enum ActionType { ATTACK = 0, CHNGE = 1, CATCH = 2, ESCAPE = 3 };
enum Priority { SLOW = 0, NORMAL = 1, FAST = 2 };

class Action {
 public:
  Priority priority;
  ActionType actionType;
  uint8_t actionIndex;

  void setActionType(ActionType type, Priority priority);
};