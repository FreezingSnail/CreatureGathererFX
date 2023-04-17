#pragma once
#include <stdint.h>

// This is an encounter action
// attack, change, item, escape
enum ActionType {
    ATTACK = 0,
    CHNGE = 1,
    ITEM = 2,
    ESCAPE = 3
};

class Action {
    public:
        uint8_t priority;
        ActionType actionType;
        uint8_t actionIndex;

        void setActionType(ActionType type);

};