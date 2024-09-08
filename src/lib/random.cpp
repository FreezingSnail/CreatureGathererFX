#include "random.hpp"
#include "Arduboy2.h"
#include "Arduino.h"

bool randomRoll(uint8_t l, uint8_t r, uint8_t target) {
    return random(l, r + 1) == target;
}

uint8_t randomRoll(uint8_t l, uint8_t r) {
    return random(l, r + 1);
}
