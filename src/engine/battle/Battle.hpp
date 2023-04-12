#pragma once
#define DEBUG
#include "creature/Creature.hpp"
#include "action/Action.hpp"
#include "engine/game/Menu.hpp"
#include "opponent/Opponent.hpp"


class BattleEngine {
    private:
        Creature* playerParty[3];
        Creature opponent[3];
        uint16_t playerHealths[3];
        uint16_t opponentHealths[3];
        uint8_t awakeMons; //11100111 player and opponet bit array
        Creature* playerCur;
        Creature* opponentCur; 
        uint8_t playerIndex;
        uint8_t opponentIndex;
        Action playerAction;
        Action opponentAction;
        Menu* menu;


    public:
        BattleEngine();

        void Encounter();

        void LoadPlayer(Creature* playerParty[3]);

        void LoadOpponent(Opponent* opponent);

        void startEncounter();

        void endEncounter();

        // sets playerAction
        void getInput();

        // Sets opponentAction
        void opponentInput();

        void turnTick();

        bool checkLoss();

        bool checkWin();

        void commitAction(Action* action, Creature* commiter, Creature* receiver) ;

        void applyDamage(uint16_t damage, Creature* receiver);



    #ifdef DEBUG
        void loadAwakeMons(uint8_t);
        void printEncounter();
    #endif


};