#pragma once
#include <stdint.h>

#include "../battle/Battle.hpp"
#include "../game/Gamestate.hpp"
#include "../menu/MenuV2.hpp"

#include "Encounter.hpp"
#include "Event.hpp"
#include "Map.hpp"
#include "TileMap.hpp"
#include <ArduboyFX.h>

#define EVENTCOUNT 6

enum class Direction { UP, RIGHT, DOWN, LEFT };

class WorldEngine {
  private:
    Direction playerDirection;
    Encounter encounterTable;
    int mapx, mapy;
    uint8_t height, width;
    int8_t up, side;
    int8_t curChunk, chunkX, chunkY;

    TileMap tMap;

    bool moving;
    uint8_t stepTicker;
    uint8_t nextTile;
    int curx, cury;

    uint8_t mapIndex;
    uint8_t submapCount;
    uint24_t submapAdders[6];

    uint8_t warps[6][4];
    uint24_t debugAdder;
    Event events[EVENTCOUNT];

  public:
    WorldEngine();
    void init();
    void input();
    void runMap();
    void draw();
    void drawMap();
    void drawEvents();
    void drawPlayer();
    void moveChar();
    uint8_t getTile();
    void encounter();
    bool moveable();
    void interact();

    void loadMap(uint8_t mapIndex, uint8_t submapIndex);
    void warp();
    void setPos(uint8_t x, uint8_t y);
    void loadEventTable();
};
