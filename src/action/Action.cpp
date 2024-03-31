#include "Action.hpp"

void Action::setActionType(ActionType type, Priority priority) {
    this->actionType = type;
    this->priority = priority;
    this->actionIndex = 0;
}
