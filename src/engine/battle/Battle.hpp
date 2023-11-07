#pragma once
#include "../../action/Action.hpp"
#include "../../creature/Creature.hpp"
#include "../../engine/game/Menu.hpp"
#include "../../opponent/Opponent.hpp"
#include "../../player/Player.hpp"
#include "../game/Gamestate.hpp"
#include <ArduboyFX.h>

class BattleEngine {
  private:
    Creature *playerParty[3];
    GameState_t *state;
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

    bool activeBattle;

  public:
    BattleEngine();
    void init(GameState_t *state);

    // entry points
    void startFight(Arduboy2 &arduboy, Player &player, Menu &menu, uint8_t optID);
    void startEncounter(Arduboy2 &arduboy, Player &player, Menu &menu, uint8_t creatureID, uint8_t level);

    // flow control
    void encounter(Arduboy2 &arduboy, Player &player, Menu &menu);

  private:
    // flow control
    void turnTick(Player &player, Menu &menu);
    bool checkLoss();
    bool checkWin();
    bool checkPlayerFaint(Menu &menu);
    bool checkOpponentFaint();
    void playerActionFirst(Player &player, Menu &menu);
    void opponentActionFirst(Player &player, Menu &menu);
    void changeCurMon(Menu &menu, uint8_t index);
    bool tryCapture();
    void endEncounter();

    // inputs
    bool getInput(Menu &menu);
    void opponentInput();

    // event execution
    void commitAction(Player &player, Menu &menu, Action *action, Creature *commiter, Creature *receiver);
    void applyDamage(uint16_t damage, Creature *receiver);
    uint16_t calculateDamage(Menu &menu, Action *action, Creature *committer, Creature *reciever);

    // data loading
    void loadPlayer(Menu &menu, Player &player);
    void loadOpponent(uint8_t optID);
    void LoadCreature(uint8_t creatureID, uint8_t level);
    void resetOpponent();

    // drawing
    void drawScene(Arduboy2 &arduboy);
    void drawPlayer(Arduboy2 &arduboy);
    void drawOpponent(Arduboy2 &arduboy);
    void drawPlayerHP(Arduboy2 &arduboy);
    void drawOpponentHP(Arduboy2 &arduboy);
};