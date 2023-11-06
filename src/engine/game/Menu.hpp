#pragma once
#include "../../action/Action.hpp"
#include "../../lib/Move.hpp"
#include "../../lib/Type.hpp"
#include "../../player/Player.hpp"
#include "../game/Gamestate.hpp"
#include <ArduboyFX.h>

enum MenuType {
    BMAIN,
    BMOVE,
    BCATCH,
    BCHANGE,

};

class Menu {
  private:
    Arduboy2 *arduboy;
    GameState_t *state;
    Player *player;
    MenuType curMenu;
    // move IDs
    uint8_t moveList[4];
    uint8_t creatureList[2];
    // pointers to the item name strings based on the current creature moves
    // char *items[32];

    int8_t cursorIndex;
    bool queued;
    Action queuedAction;

  public:
    Menu();
    Menu(Arduboy2 *arduboy, GameState_t *state, Player *player);
    void registerMoveList(uint8_t move1, uint8_t move2, uint8_t move3, uint8_t move4);
    void registerCreatureList(uint8_t c1, uint8_t c2);
    bool actionInput(Action *action);
    void wait();

    void drawInfoRec();
    void printMenu();

    // Arena menus
    void creatureRental();
    int8_t registerCreature();

    // battle menus
    void printBattleMenu();
    void printMoveMenu();
    void printItemMenu();

    // world menus
    void printWorldMenu();
    void printCreatureMenu();
    void printInventoryMenu();

    void printCursor();
    void moveCursor();

    void printAttack(uint8_t creatureID, uint8_t attackID, Modifier modifier);

    void transverseMenu();
    void tansverseBattleMenu();
    void queueAction(ActionType type, uint8_t index);
};