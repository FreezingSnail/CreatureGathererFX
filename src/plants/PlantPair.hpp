#pragma once
#include <stdint.h>

enum PlantType {
    BERRY,
    FLOWER,
    APPLE,
    A,
    B,
    C,
    D,
    E,
};

// 0 000
// | type
// active

class PlantPair {
  private:
    uint8_t value = 0;

  public:
    bool isActive(uint8_t index);
    PlantType type(uint8_t index);

    void setActive(uint8_t index, bool active);
    void setType(uint8_t index, PlantType type);
};
