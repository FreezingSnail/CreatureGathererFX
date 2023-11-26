#pragma once
#include "../../player/Player.hpp"
#include "../battle/Battle.hpp"
#include "../game/Menu.hpp"
#include "../menu/MenuV2.hpp"

class Arena {
  private:
    uint8_t registerIndex;

    uint8_t moveIndex;
    uint8_t moveCreature;
    uint8_t cursor;
    uint8_t movePointer;
    uint32_t debug;

  public:
    Arena() = default;
    void arenaLoop(Arduboy2 &arduboy, Menu &menu, MenuV2 &menu2, Player &player, BattleEngine &engine);
    void registerRentals(Menu &menu, Player &player);
    void registerMoves(Arduboy2 &arduboy, Player &player);
    uint8_t selectOpponent();
    void startBattle(Arduboy2 &arduboy, BattleEngine &engine, Player &player, Menu &menu);
    void displayRegisteredCount(Arduboy2 &arduboy);
};