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

        bool activeBattle;
        


    public:
        BattleEngine(Arduboy2* arduboy, Menu* menu);

        void encounter();

        void loadPlayer(Player* player);

        void loadOpponent(uint8_t optID);

        void startEncounter(Player* player, uint8_t optID);

        void endEncounter();

        // sets playerAction
        bool getInput();

        // Sets opponentAction
        void opponentInput();

        void turnTick();

        bool checkLoss();

        bool checkWin();

        void commitAction(Action* action, Creature* commiter, Creature* receiver) ;

        void applyDamage(uint16_t damage, Creature* receiver);

        void playerActionFirst();
        void opponentActionFirst();
        bool checkPlayerFaint();
        bool checkOpponentFaint();
        uint16_t calculateDamage(Action* action, Creature* committer, Creature* reciever);

        void drawScene();
        void drawPlayer();
        void drawOpponent();
        void drawPlayerHP();
        void drawOpponentHP();


    #ifdef DEBUG
        void printEncounter();
    #endif


};