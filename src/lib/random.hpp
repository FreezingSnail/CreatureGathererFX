#pragma once

#include <stdint.h>

void rngSeed(uint16_t seed);
uint8_t rngNext8();
bool randomRoll(uint8_t l, uint8_t r, uint8_t target);
uint8_t randomRoll(uint8_t l, uint8_t r);
