#include "TileMap.hpp"
#include <ArduboyFX.h>
#include "../../fxdata/fxdata.h"

#define TILE_WIDTH 16
#define TILE_HEIGHT 16

TileMap::TileMap() {
    this->originX = 0;
    this->originY = 0;
    for (uint8_t i = 0; i < 6; i++) {
        this->map[i] = &this->lines[i];
    }
    mapN = 24, mapM = 24;
}

void TileMap::shiftUp() {
    this->originY--;
    TileLine *temp = this->map[0];
    for (uint8_t i = 0; i < 5; i++) {
        this->map[i] = this->map[i + 1];
    }
    this->map[5] = temp;
    this->loadLine(5, this->originX, this->originY + 5);
}

void TileMap::shiftDown() {
    this->originY++;
    TileLine *temp = this->map[5];
    for (uint8_t i = 5; i > 0; i--) {
        this->map[i] = this->map[i - 1];
    }
    this->map[0] = temp;
    this->loadLine(0, this->originX, this->originY);
}

void TileMap::shiftLeft() {
    this->originX--;
    for (uint8_t i = 0; i < 6; i++) {
        for (uint8_t j = 0; j < 9; j++) {
            this->map[i]->tiles[j] = this->map[i]->tiles[j + 1];
        }
        this->map[i]->tiles[9] = 0;
    }
    for (uint8_t i = 0; i < 6; i++) {
        this->loadCell(i, 9);
    }
}

void TileMap::shiftRight() {
    this->originX++;
    for (uint8_t i = 0; i < 6; i++) {
        for (uint8_t j = 9; j > 0; j--) {
            this->map[i]->tiles[j] = this->map[i]->tiles[j - 1];
        }
        this->map[i]->tiles[0] = 0;
    }
    for (uint8_t i = 0; i < 6; i++) {
        this->loadCell(i, 0);
    }
}

void TileMap::loadLine(uint8_t index, uint8_t startX, uint8_t startY) {
    for (uint8_t i = 0; i < 10; i++) {
        this->lines[index].tiles[i];
    }
}

void TileMap::loadCell(uint8_t x, uint8_t y) {
    uint8_t targetX = this->originX + x;
    uint8_t targetY = this->originY + y;
    uint16_t dataIndex = y * mapM + x;

    // the index is a 16 bit width address but only 8bit width is allowed
    // need to convert 16 bit to 8 bit + offset
    // i.e. 256 -> 1,0 257 -> 1,1
    uint8_t offset = dataIndex >> 8;
    uint8_t integer = dataIndex & 0xFF;
    this->map[x]->tiles[y] = FX::readIndexedUInt8(testmap + (offset * 256), integer);
}

void TileMap::draw(uint8_t offsetX, uint8_t offsetY) {
    for (uint8_t i = 0; i < 6; i++) {
        for (uint8_t j = 0; j < 10; j++) {
            uint8_t x = (j * TILE_WIDTH) - offsetX;
            uint8_t y = (i * TILE_HEIGHT) - offsetY;
            FX::drawBitmap(x, y, tilesheet, map[i]->tiles[j], dbmNormal);
        }
    }
}