#pragma once
#include <stdint.h>

class PlantStage {
  private:
    // 2bit per index
    // lowest 2 bits are index 0
    // highest 2 bits index 31
    uint64_t value = 0;

  public:
    void increment(uint8_t index);
    void incrementAll();
    uint8_t read(uint8_t index);
};