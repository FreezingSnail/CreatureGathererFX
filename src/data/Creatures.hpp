#pragma once
#include <avr/pgmspace.h>

// todo research huffman encoding to squash these in mem
typedef struct CreatureD {
  unsigned char id;
  unsigned char type1;
  unsigned char type2;
  unsigned char evoLevel;
  unsigned char atkSeed;
  unsigned char defSeed;
  unsigned char spcAtkSeed;
  unsigned char spcDefSeed;
  unsigned char hpSeed;
  unsigned char spdSeed;
  unsigned char move1;
  unsigned char move2;
  unsigned char move3;
  unsigned char move4;
} CreatureData_t;
