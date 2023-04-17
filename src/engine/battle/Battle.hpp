#pragma once
#include "Arduboy2.h"
#include "../../creature/Creature.hpp"
#include "../../action/Action.hpp"
#include "../../engine/game/Menu.hpp"
#include "../../opponent/Opponent.hpp"
#include "../../player/Player.hpp"
#define DEBUG


class BattleEngine {
    private:
        Arduboy2 * arduboy;
        Creature* playerParty[3];
        Opponent opponent;
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
        BattleEngine(Arduboy2* arduboy);

        void encounter(Player* player, OpponentSeed* seed);

        void loadPlayer(Player* player);

        void loadOpponent(OpponentSeed* seed);

        void startEncounter(Player* player, OpponentSeed* seed);

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

        void playerActionFirst();
        void opponentActionFirst();
        void checkPlayerFaint();
        void checkOpponentFaint();
        uint16_t calculateDamage(Action* action, Creature* committer, Creature* reciever);


    #ifdef DEBUG
        void printEncounter();
    #endif


};