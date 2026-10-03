#pragma once
#include "../../player/Player.hpp"
#include "../battle/Battle.hpp"
#include "../menu/MenuV2.hpp"

class Arena {
  private:
    uint8_t registerIndex;

    uint8_t moveIndex;
    uint8_t moveCreature;
    uint8_t cursor;
    uint8_t movePointer;
    uint8_t cachedPoolCreatureId = 255;
    uint8_t cachedMovePointer = 255;
    uint8_t cachedInfoMove = 255;
    uint32_t movePool = 0;
    uint24_t visibleMoveAddresses[4] = {};
    Move selectedMove;

    void prepareMovePool(Player &player);
    void prepareVisibleMoves();

  public:
    Arena() = default;
    bool arenaLoop(MenuV2 &menu2, Player &player);
    void registerRentals(Player &player, MenuV2 &menu2);
    void registerMoves(Player &player);
    void drawregisterMoves(Player &player);
    uint8_t selectOpponent();
    void startBattle(BattleEngine &engine, Player &player, MenuV2 &menu2);
    void displayRegisteredCount();
    void drawarenaLoop(MenuV2 &menu2, Player &player);
};
