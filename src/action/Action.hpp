#pragma once

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
        ActionType_t actionType;

        void setActionType(ActionType_t);

};