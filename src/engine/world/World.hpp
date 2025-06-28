#pragma once
#include <stdint.h>

#include <ArduboyFX.h>

#define EVENTCOUNT 6

enum class Direction {
    UP,
    RIGHT,
    DOWN,
    LEFT
};

class WorldEngine {
  private:
    Direction playerDirection;
    uint8_t mapx, mapy;
    uint8_t height, width;
    int8_t up, side;
    int8_t curChunk, chunkX, chunkY;

    bool moving;
    uint8_t stepTicker;
    uint8_t nextTile;
    int curx, cury;

  public:
    WorldEngine();
    void init();
    void input();
    void runMap();
    void moveChar();
    uint8_t getTile();
    void encounter();
    bool moveable();
    void interact();

    void loadMap(uint8_t mapIndex, uint8_t submapIndex);
    void setPos(uint8_t x, uint8_t y);
};
