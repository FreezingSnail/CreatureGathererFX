#include "TileMap.hpp"
#include "../../common.hpp"
#include <ArduboyFX.h>
#include "../../fxdata.h"

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
    this->originY++;
    TileLine *temp = this->map[0];
    for (uint8_t i = 0; i < 5; i++) {
        this->map[i] = this->map[i + 1];
    }
    this->map[5] = temp;
    if (originY <= mapN) {
        this->loadLine(5, this->originX, this->originY + 5);
    } else {
        for (uint8_t i = 0; i < 10; i++) {
            this->map[5]->tiles[i] = 0;
        }
    }
}

void TileMap::shiftDown() {
    this->originY--;
    TileLine *temp = this->map[5];
    for (uint8_t i = 5; i > 0; i--) {
        this->map[i] = this->map[i - 1];
    }
    this->map[0] = temp;
    if (originY >= 0) {
        this->loadLine(0, this->originX, this->originY);
    } else {
        for (uint8_t i = 0; i < 10; i++) {
            this->map[0]->tiles[i] = 0;
        }
    }
}

void TileMap::shiftLeft() {
    this->originX++;
    for (uint8_t i = 0; i < 6; i++) {
        for (uint8_t j = 0; j < 9; j++) {
            this->map[i]->tiles[j] = this->map[i]->tiles[j + 1];
            map[i]->debug[j] = map[i]->debug[j + 1];
        }
        this->map[i]->tiles[9] = 0;
    }
    if (originX + 9 <= mapM) {
        loadVLine(9, this->originX + 9, this->originY);
    } else {
        for (uint8_t i = 0; i < 6; i++) {
            this->map[i]->tiles[9] = 0;
        }
    }
}

void TileMap::shiftRight() {
    this->originX--;
    for (uint8_t i = 0; i < 6; i++) {
        for (uint8_t j = 9; j > 0; j--) {
            this->map[i]->tiles[j] = this->map[i]->tiles[j - 1];
            map[i]->debug[j] = map[i]->debug[j - 1];
        }
        this->map[i]->tiles[0] = 0;
    }

    if (originX >= 0) {
        loadVLine(0, this->originX, this->originY);
    } else {
        for (uint8_t i = 0; i < 6; i++) {
            this->map[i]->tiles[0] = 0;
        }
    }
}

void TileMap::loadMap(uint8_t topX, uint8_t topY) {
    this->originX = topX;
    this->originY = topY;
    for (uint8_t i = 0; i < 6; i++) {
        for (uint8_t j = 0; j < 10; j++) {
            this->loadCell(topX + i, topY + j);
        }
    }
}

void TileMap::loadLine(uint8_t index, uint16_t startX, uint16_t startY) {
    uint16_t dataIndex = startY * mapM + startX;
    // the index is a 16 bit width address but only 8bit width is allowed
    // need to convert 16 bit to 8 bit + offset
    // i.e. 256 -> 1,0 257 -> 1,1
    uint8_t offset = dataIndex >> 8;
    uint8_t integer = dataIndex & 0xFF;
    uint24_t addr = testmap + (offset * 256) + integer * sizeof(uint8_t);
    FX::seekData(addr);
    for (uint8_t i = 0; i < 10; i++) {
        this->map[index]->tiles[i] = FX::readPendingUInt8();
        map[index]->debug[i] = dataIndex;
    }
    (void)FX::readEnd();
}

void TileMap::loadVLine(uint8_t index, uint16_t startX, uint16_t startY) {
    for (uint8_t i = 0; i < 6; i++) {
        uint16_t dataIndex = (startY + i) * mapM + startX;
        // the index is a 16 bit width address but only 8bit width is allowed
        // need to convert 16 bit to 8 bit + offset
        // i.e. 256 -> 1,0 257 -> 1,1
        uint8_t offset = dataIndex >> 8;
        uint8_t integer = dataIndex & 0xFF;
        uint24_t addr = testmap + (offset * 256) + (integer + mapM) * sizeof(uint8_t);
        map[i]->tiles[index] = FX::readIndexedUInt8(testmap + (offset * 256), integer);
        map[i]->debug[index] = dataIndex;
    }
}

void TileMap::loadCell(uint16_t x, uint16_t y) {
    uint16_t targetX = this->originX + x;
    uint16_t targetY = this->originY + y;
    uint16_t dataIndex = x * mapM + y;

    // the index is a 16 bit width address but only 8bit width is allowed
    // need to convert 16 bit to 8 bit + offset
    // i.e. 256 -> 1,0 257 -> 1,1
    uint8_t offset = dataIndex >> 8;
    uint8_t integer = dataIndex & 0xFF;
    this->map[x]->tiles[y] = FX::readIndexedUInt8(testmap + (offset * 256), integer);
    this->map[x]->debug[y] = dataIndex;
}

void TileMap::draw(int8_t pX, int8_t pY, int8_t offsetX, int8_t offsetY) {
    for (uint8_t i = 0; i < 6; i++) {
        for (uint8_t j = 0; j < 10; j++) {
            int8_t x = ((j * TILE_WIDTH) - (pX * TILE_WIDTH) - TILE_WIDTH) + offsetX;
            int8_t y = ((i * TILE_HEIGHT) - (pY * TILE_HEIGHT) - TILE_HEIGHT) + offsetY;
            SpritesU::drawOverwriteFX(x, y, worldTiles, this->map[i]->tiles[j] * 3);
        }
    }
}