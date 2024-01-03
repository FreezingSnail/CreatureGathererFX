#pragma once
#include <stdint.h>

#include "../battle/Battle.hpp"
#include "../game/Gamestate.hpp"
#include "../menu/MenuV2.hpp"

#include "Encounter.hpp"
#include "Event.hpp"
#include "Map.hpp"
#include "Chunk.hpp"
#include <ArduboyFX.h>

#define EVENTCOUNT 6

// Notes: character center on map at all times
enum Direction { Up, Right, Down, Left };

class WorldEngine {
  private:
    Arduboy2Base *arduboy;
    Direction playerDirection;
    Encounter encounterTable;
    BattleEngine *battleEngine;
    MenuV2 *menu2;
    int mapx, mapy;
    uint8_t height, width;

    bool moving;
    uint8_t stepTicker;
    GameState_t *state;
    uint8_t nextTile;
    int curx, cury;

    uint8_t mapIndex;
    uint8_t submapCount;
    uint24_t submapAdders[6];

    uint8_t warps[6][4];
    uint24_t debugAdder;
    Event events[EVENTCOUNT];
    Chunk chunk[9];

  public:
    WorldEngine();
    void init(Arduboy2Base *arduboy, GameState_t *state, BattleEngine *battleEngine, MenuV2 *menu2);
    void input();
    void runMap(Player *player);
    void drawMap();
    void drawChunk();
    void drawEvents();
    void drawPlayer();
    void moveChar(Player *player);
    uint8_t getTile();
    void encounter(Arduboy2Base *arduboy, Player *player);
    bool moveable();
    void interact();

    void loadMap(uint8_t mapIndex, uint8_t submapIndex);
    void warp();
    void setPos(uint8_t x, uint8_t y);
    void loadEventTable();
};
