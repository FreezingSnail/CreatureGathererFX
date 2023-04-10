#pragma once
#define DEBUG
#include "creature/Creature.hpp"
#include "action/Action.hpp"
#include "game/Menu.hpp"


class BattleEngine {
    private:
        Creature* playerParty[3];
        Creature opponent[3];
        int playerHealths[3];
        int oponentHealths[3];
        uint8_t awakeMons; //11100111 player and opponet bit array
        Creature* playerCur;
        Creature* opponentCur;
        Action playerAction;
        Action opponentAction;
        Menu* menu;


    public:
        BattleEngine();

        void Encounter();

        void LoadPlayer(Creature* playerParty[3]);

        void LoadOpponent();

        void startEncounter();

        void endEncounter();

        // sets playerAction
        void getInput();

        // Sets opponentAction
        void opponentInput();

        void turnTick();

        bool checkLoss();

        bool checkWin();



    #ifdef DEBUG
        void loadAwakeMons(uint8_t);
        void printEncounter();
    #endif


};