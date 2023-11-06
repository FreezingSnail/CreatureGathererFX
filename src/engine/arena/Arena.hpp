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
    Arduboy2 *arduboy;

    uint8_t moveIndex;
    uint8_t moveCreature;
    uint8_t cursor;
    uint32_t debug;

  public:
    Arena();
    Arena(Menu *menu, Player *player, BattleEngine *engine, Arduboy2 *arduboy);
    void arenaLoop(Arduboy2 *arduboy);
    void registerRentals();
    void registerMoves();
    uint8_t selectOpponent();
    void startBattle();
    void displayRegisteredCount(Arduboy2 *arduboy);
};