#pragma once
#include <avr/pgmspace.h>
#include <stdint.h>

typedef struct EncounterTable {
  unsigned char c1 : 5;
  unsigned char c2 : 5;
  unsigned char c3 : 5;
  unsigned char c4 : 5;
  // 4bit encounter rates
  unsigned char rate1 : 4;
  unsigned char rate2 : 4;
  unsigned char rate3 : 4;
  unsigned char rate4 : 4;
  unsigned char avgLevel : 5;

} EncounterTable_t;

const EncounterTable_t encounterRates[8] PROGMEM = {
    {1, 2, 3, 4, 3, 3, 3, 1, 3}, {1, 2, 3, 4, 3, 3, 3, 1, 3},
    {1, 2, 3, 4, 3, 3, 3, 1, 3}, {1, 2, 3, 4, 3, 3, 3, 1, 3},
    {1, 2, 3, 4, 3, 3, 3, 1, 3}, {1, 2, 3, 4, 3, 3, 3, 1, 3},
    {1, 2, 3, 4, 3, 3, 3, 1, 3}, {1, 2, 3, 4, 3, 3, 3, 1, 3},
};
