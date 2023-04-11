#pragma once
#include "action/Action.hpp"

class Menu{
    enum state {
        WORLD,
        BATTLE
    };

    public:
        Menu();
        void actionInput(Action* action);
};