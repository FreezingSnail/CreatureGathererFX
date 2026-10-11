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
    uint8_t serialize() const { return value; }
    void restore(uint8_t byte) { value = byte; }
    bool isActive(uint8_t index);
    PlantType type(uint8_t index);

    void setActive(uint8_t index, bool active);
    void setType(uint8_t index, PlantType type);
};
