#pragma once
#include "../src/common.hpp"

enum class PlantType {
    BERRY,
};

class PlantTile {
  public:
    Plant *plant;
    int16_t x, y;
    uint8_t stage;
    PlantType type;

    void draw(int16_t x, int16_t y);
};