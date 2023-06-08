#pragma once
#include <avr/pgmspace.h>
#include <stdint.h>

typedef struct EncounterTable {
  // bool table for encounterable creatures using creatureID
  uint32_t creatures;
  //   unsigned char c1 : 5;
  //   unsigned char c2 : 5;
  //   unsigned char c3 : 5;
  //   unsigned char c4 : 5;
  // 4bit encounter rates
  unsigned char rate1 : 4;
  unsigned char rate2 : 4;
  unsigned char rate3 : 4;
  unsigned char rate4 : 4;
  unsigned char avgLevel : 5;

} EncounterTable_t;

const EncounterTable_t encounterRates[8] PROGMEM = {
    {0x10010010001000000000000000000000, 3, 3, 3, 1, 3},
    {0x01001001000100000000000000000000, 3, 3, 3, 1, 3},
    {0x00100100100010000000000000000000, 3, 3, 3, 1, 3},
    {0x00010010010001000000000000000000, 3, 3, 3, 1, 3},
    {0x00001001001000100000000000000000, 3, 3, 3, 1, 3},
    {0x00000100100100010000000000000000, 3, 3, 3, 1, 3},
    {0x00000010010010001000000000000000, 3, 3, 3, 1, 3},
    {0x00000001001001000100000000000000, 3, 3, 3, 1, 3},
};
