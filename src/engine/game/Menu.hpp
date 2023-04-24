#pragma once
#include "Arduboy2.h"
#include "../../action/Action.hpp"

typedef enum State {
    WORLD,
    BATTLE
}State_t;

class Menu{

    private:
        Arduboy2* arduboy;
        State_t state;
        // pointers to the move name strings based on the current creature moves
        char* moveList[4];
        // pointers to the item name strings based on the current creature moves
        char* items[32];

        uint8_t cursorIndex;

    public:
        Menu(Arduboy2* arduboy);
        void actionInput(Action* action);
        void setState(State_t s);
        void printCursor();
        void printMenu();
        
        //battle menus
        void printBattleMenu();
        void printMoveMenu();
        void printItemMenu();

        //world menus
        void printWorldMenu();
        void printCreatureMenu();
        void printInventoryMenu();
        
};