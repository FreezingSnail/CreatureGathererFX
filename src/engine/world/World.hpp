#pragma once
#include <stdint.h>

#include "Chunk.hpp"

#define EVENTCOUNT 6

enum class Direction {
    UP,
    RIGHT,
    DOWN,
    LEFT
};

struct ViewOffset {
    int8_t x;
    int8_t y;
    uint8_t mask;
};

class WorldEngine {
  private:
    Direction playerDirection;
    uint8_t height, width;
    int8_t stepOffsetX, stepOffsetY;
    uint8_t walkMask;
    uint16_t lastChunk;

    void onChunkChange(uint16_t newChunk);

    bool moving;
    uint8_t stepTicker;
    uint8_t curx, cury;

  public:
    WorldEngine() = default;
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
    ViewOffset view() const;
    uint16_t location() const;
    void syncFromLocation();
#ifdef TEST
    void beginMoveForTest(Direction direction);
#endif
};
