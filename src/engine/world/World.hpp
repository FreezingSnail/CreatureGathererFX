#pragma once
#include <stdint.h>

#include "../battle/Battle.hpp"
#include "../game/Gamestate.hpp"
#include "Encounter.hpp"
#include "Event.hpp"
#include "Map.hpp"
#include <ArduboyFX.h>

// Notes: character center on map at all times
enum Direction { Up, Right, Down, Left };

class WorldEngine {
  private:
    Arduboy2 *arduboy;
    Direction playerDirection;
    Encounter encounterTable;
    BattleEngine *battleEngine;
    int mapx, mapy;
    uint8_t height, width;

    bool moving;
    uint8_t stepTicker;
    GameState_t *state;
    uint8_t nextTile;
    int curx, cury;
    int debug;

    uint8_t mapIndex;
    uint8_t submapCount;
    uint24_t submapAdders[6];

    uint8_t warps[6][4];
    uint24_t debugAdder;
    Event events[6];

  public:
    WorldEngine();
    WorldEngine(Arduboy2 *arduboy, GameState_t *state, BattleEngine *battleEngine);
    void input();
    void runMap();
    void drawMap();
    void drawPlayer();
    void moveChar();
    uint8_t getTile();
    void encounter();
    bool moveable();

    void loadMap(uint8_t mapIndex, uint8_t submapIndex);
    void warp();
    void setPos(uint8_t x, uint8_t y);
    void loadEventTable();
};
