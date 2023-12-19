#pragma once
#include "../src/action/Action.hpp"
#include "creature/Creature.hpp"
#include "opponent/Opponent.hpp"
#include "player/Player.hpp"

class MenuV2;

class BattleEngine {
  public:
    Creature *playerParty[3];
    Creature *playerCur;
    Creature *opponentCur;

    Opponent opponent;
    uint16_t playerHealths[3];
    uint16_t opponentHealths[3];
    uint8_t awakeMons;   // 11100111 player and opponet bit array

    uint8_t playerIndex;
    uint8_t opponentIndex;
    Action playerAction;
    Action opponentAction;

    bool queued;
    Action queuedAction;

    bool activeBattle;
    void init();

    // entry points
    void startFight(Player &player, uint8_t optID);
    void startArena(Player &player, uint8_t optID);
    void startEncounter(Player &player, uint8_t creatureID, uint8_t level);

    // flow control
    void encounter(Player &player);

    // menu interface
    void queueAction(ActionType type, uint8_t index);

    uint8_t *getPlayerCurCreatureMoves();
    void updateInactiveCreatures(uint8_t *list);
    Creature *getCreature(uint8_t index);

    // flow control
    void turnTick(Player &player);
    bool checkLoss();
    bool checkWin();
    bool checkPlayerFaint();
    bool checkOpponentFaint();
    void playerActionFirst(Player &player);
    void opponentActionFirst(Player &player);
    void setMoveList(uint8_t **pointer);
    void changeCurMon(uint8_t index);
    bool tryCapture();
    void endEncounter();

    // inputs
    bool getInput();
    void opponentInput();

    // event execution
    void commitAction(Player &player, Action *action, Creature *commiter, Creature *receiver, bool isPlayer);
    void applyDamage(uint16_t damage, Creature *receiver);

    // data loading
    void loadPlayer(Player &player);
    void loadOpponent(uint8_t optID);
    void LoadCreature(uint8_t creatureID, uint8_t level);
    void resetOpponent();

    // drawing
    void drawScene();
    void drawPlayer();
    void drawOpponent();
    void drawPlayerHP();
    void drawOpponentHP();
};