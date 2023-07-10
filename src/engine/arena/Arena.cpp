#include "Arena.hpp"
#include "../../player/Player.hpp"
#include "../battle/Battle.hpp"
#include "../game/Menu.hpp"

Arena::Arena(Menu *menu, Player *player, BattleEngine *engine) {
  this->menu = menu;
  this->player = player;
  this->engine = engine;
}

void Arena::arenaLoop() {
  this->menu->printMenu();
  if (this->registerIndex < 3) {
    this->registerRentals();
  } else {
    this->startBattle();
  }
}

void Arena::registerRentals() {
  int8_t creatureID = this->menu->registerCreature();
  if (creatureID >= 0) {
    this->player->loadreature(this->registerIndex, creatureID);
    this->registerIndex++;
  }
}

uint8_t Arena::selectOpponent() { return 0; }
void Arena::startBattle() { this->engine->startFight(this->selectOpponent()); }