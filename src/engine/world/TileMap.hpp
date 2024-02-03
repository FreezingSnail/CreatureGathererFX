#pragma once
#include <stdint.h>

struct TileLine {
    uint8_t tiles[10];
    uint16_t debug[10];
};

class TileMap {
  public:
    TileLine lines[6];
    TileLine *map[6];
    int8_t originX, originY;
    uint8_t mapM, mapN;

    TileMap();
    void shiftUp();
    void shiftDown();
    void shiftLeft();
    void shiftRight();

    void loadMap(uint8_t topX, uint8_t topY);
    void loadLine(uint8_t index, uint16_t startX, uint16_t startY);
    void loadVLine(uint8_t index, uint16_t startX, uint16_t startY);
    void loadCell(uint16_t x, uint16_t y);

    void draw(int8_t x, int8_t y, int8_t offsetX, int8_t offsetY);
};
