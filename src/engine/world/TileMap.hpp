#pragma once
#include <cstdint>

struct TileLine {
    uint8_t tiles[10];
};

class TileMap {
    TileLine lines[6];
    TileLine *map[6];
    uint8_t originX, originY;
    uint8_t mapM, mapN;

    void shiftUp();
    void shiftDown();
    void shiftLeft();
    void shiftRight();

    void loadLine(uint8_t index, uint8_t startX, uint8_t startY);
    void loadCell(uint8_t x, uint8_t y);

    void draw(uint8_t offsetX, uint8_t offsetY);
};
