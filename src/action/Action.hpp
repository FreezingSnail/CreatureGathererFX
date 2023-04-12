#pragma once
#include <stdint.h>

// This is an encounter action
// attack, change, item, escape
typedef enum ActionType {
    ATTACK,
    CHANGE,
    ITEM,
    ESCAPE
}ActionType_t;

class Action {
    public:
        uint8_t priority;
        ActionType_t actionType;
        uint8_t actionIndex;

        void setActionType(ActionType_t);

};