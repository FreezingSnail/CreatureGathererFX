#pragma once
#include "../../action/Action.hpp"
#include "../../creature/Creature.hpp"
#include "../../engine/game/Menu.hpp"
#include "../../opponent/Opponent.hpp"
#include "../../player/Player.hpp"
#include "../game/Gamestate.hpp"
#include "Arduboy2.h"

#define DEBUG

class BattleEngine {
private:
  Arduboy2 *arduboy;
  Creature *playerParty[3];
  Player *player;
  Opponent opponent;
  uint16_t playerHealths[3];
  uint16_t opponentHealths[3];
  uint8_t awakeMons; // 11100111 player and opponet bit array

  Creature *playerCur;
  Creature *opponentCur;

  uint8_t playerIndex;
  uint8_t opponentIndex;
  Action playerAction;
  Action opponentAction;
  Menu *menu;
  GameState *state;

  bool activeBattle;

  int db;

public:
  BattleEngine(Arduboy2 *arduboy, Player *player, Menu *menu, GameState *state);

  // entry points
  void startFight(uint8_t optID);
  void startEncounter(uint8_t creatureID, uint8_t level);

  // flow control
  void encounter();

private:
  // flow control
  void turnTick();
  bool checkLoss();
  bool checkWin();
  bool checkPlayerFaint();
  bool checkOpponentFaint();
  void playerActionFirst();
  void opponentActionFirst();
  void changeCurMon(uint8_t index);
  bool tryCapture();
  void endEncounter();

  // inputs
  bool getInput();
  void opponentInput();

  // event execution
  void commitAction(Action *action, Creature *commiter, Creature *receiver);
  void applyDamage(uint16_t damage, Creature *receiver);
  uint16_t calculateDamage(Action *action, Creature *committer,
                           Creature *reciever);

  // data loading
  void loadPlayer(Player *player);
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