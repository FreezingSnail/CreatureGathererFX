#pragma once
#include "../../player/Player.hpp"
#include "../battle/Battle.hpp"
#include "../game/Menu.hpp"

class Arena {
private:
  uint8_t registerIndex;
  Menu *menu;
  Player *player;
  BattleEngine *engine;

public:
  Arena(Menu *menu, Player *player, BattleEngine *engine);
  void arenaLoop(Arduboy2 *arduboy);
  void registerRentals();
  uint8_t selectOpponent();
  void startBattle();
  void displayRegisteredCount(Arduboy2 *arduboy);
};