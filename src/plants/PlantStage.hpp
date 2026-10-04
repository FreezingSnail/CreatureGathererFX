#pragma once
#include <stdint.h>

class PlantStage {
  private:
    // 2bit per index
    // lowest 2 bits are index 0
    // highest 2 bits index 31
    uint8_t value[8] = {};

  public:
    void increment(uint8_t index);
    void incrementAll();
    uint8_t getStage(uint8_t index);
};

static_assert(sizeof(PlantStage) == 8, "PlantStage save layout must remain eight bytes");