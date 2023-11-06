#pragma once
#include <stdint.h>

#include "../battle/Battle.hpp"
#include "../game/Gamestate.hpp"
#include "../game/MenuV2.hpp"

#include "Encounter.hpp"
#include "Event.hpp"
#include "Map.hpp"
#include <ArduboyFX.h>

#define EVENTCOUNT 6

// Notes: character center on map at all times
enum Direction { Up, Right, Down, Left };

class WorldEngine {
  private:
    Arduboy2 *arduboy;
    Direction playerDirection;
    Encounter encounterTable;
    BattleEngine *battleEngine;
    MenuV2 *menu2;
    uint8_t mapx, mapy;
    uint8_t height, width;

    bool moving;
    uint8_t stepTicker;
    GameState_t *state;
    uint8_t nextTile;
    uint8_t curx, cury;

    uint8_t mapIndex;
    uint8_t submapCount;
    uint24_t submapAdders[6];

    uint8_t warps[6][4];
    uint24_t debugAdder;
    Event events[EVENTCOUNT];

  public:
    WorldEngine();
    WorldEngine(Arduboy2 *arduboy, GameState_t *state, BattleEngine *battleEngine, MenuV2 *menu2);
    void input();
    void runMap(Player *player, Menu *menu);
    void drawMap();
    void drawEvents();
    void drawPlayer();
    void moveChar(Player *player, Menu *menu);
    uint8_t getTile();
    void encounter(Arduboy2 *arduboy, Player *player, Menu *menu);
    bool moveable();
    void interact();

    void loadMap(uint8_t mapIndex, uint8_t submapIndex);
    void warp();
    void setPos(uint8_t x, uint8_t y);
    void loadEventTable();
};
