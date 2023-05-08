#pragma once
#include "Arduboy2.h"
#include "../../action/Action.hpp"
#include "../../lib/Type.hpp"

typedef enum State {
    WORLD,
    BATTLE
}State_t;

enum MenuType {
    BMAIN,
    BMOVE,
    BITEM,
    BCHANGE,

};

class Menu{

    private:
        Arduboy2* arduboy;
        State_t state;
        MenuType curMenu;
        // pointers to the move name strings based on the current creature moves
        uint8_t moveList[4];
        // pointers to the item name strings based on the current creature moves
        char* items[32];

        uint8_t cursorIndex;
        bool queued;
        Action queuedAction;

    public:
        Menu(Arduboy2* arduboy);
        void registerMoveList(uint8_t move1, uint8_t move2, uint8_t move3, uint8_t move4);
        bool actionInput(Action* action);
        void setState(State_t s);
        void wait();
        void printMenu();
        
        //battle menus
        void printBattleMenu();
        void printMoveMenu();
        void printItemMenu();

        //world menus
        void printWorldMenu();
        void printCreatureMenu();
        void printInventoryMenu();

        void printCursor();
        void moveCursor();

        void printAttack(uint8_t creatureID, uint8_t attackID, Modifier modifier);
        
        void tansverseMenu();
        void queueAction(ActionType type, uint8_t index);
};