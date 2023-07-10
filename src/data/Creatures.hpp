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

// going to need to embed these into progmem at somepoint
// each instance wastes 4 bits

const CreatureData_t emptyCreature PROGMEM = {0, 0, 0, 0, 0, 0, 0,
                                              0, 0, 0, 0, 0, 0, 0};

const CreatureData_t CreatureData[32] PROGMEM = {
    {0, 2, 8, 8, 3, 3, 2, 2, 3, 3, 8, 32, 32, 32},
    {1, 2, 8, 16, 8, 8, 7, 7, 8, 8, 8, 9, 32, 32},
    {2, 2, 6, 24, 12, 12, 10, 10, 12, 12, 8, 9, 26, 10},
    {3, 3, 8, 8, 2, 3, 2, 3, 4, 2, 13, 32, 32, 32},
    {4, 3, 8, 16, 6, 9, 7, 9, 9, 6, 13, 20, 32, 32},
    {5, 3, 5, 24, 10, 13, 11, 13, 13, 8, 13, 20, 21, 12},
    {6, 1, 8, 8, 2, 2, 3, 3, 3, 3, 4, 32, 32, 32},
    {7, 1, 8, 16, 7, 7, 8, 8, 8, 8, 4, 7, 32, 32},
    {8, 1, 4, 24, 10, 10, 12, 12, 12, 12, 4, 7, 17, 6},
    {9, 0, 8, 0, 8, 8, 3, 3, 9, 6, 1, 32, 32, 32},
    {10, 4, 8, 0, 3, 3, 8, 8, 4, 9, 19, 32, 32, 32},
    {11, 5, 8, 0, 3, 3, 8, 8, 4, 7, 20, 32, 32, 32},
    {12, 6, 8, 0, 8, 8, 3, 3, 6, 4, 8, 32, 32, 32},
    {13, 2, 8, 0, 8, 8, 3, 3, 8, 8, 10, 23, 32, 32},
    {14, 3, 8, 0, 4, 10, 6, 12, 11, 2, 13, 2, 32, 32},
    {15, 3, 8, 14, 2, 2, 7, 4, 4, 8, 13, 17, 32, 32},
    {16, 3, 4, 0, 4, 4, 10, 6, 6, 12, 12, 17, 9, 32},
    {17, 1, 8, 14, 8, 3, 7, 8, 6, 11, 4, 24, 32, 32},
    {18, 1, 6, 0, 11, 4, 9, 12, 8, 13, 4, 24, 6, 27},
    {19, 2, 8, 14, 10, 8, 7, 6, 8, 8, 11, 20, 32, 32},
    {20, 2, 5, 0, 13, 10, 9, 8, 10, 12, 11, 20, 23, 10},
    {21, 0, 8, 14, 8, 8, 8, 8, 8, 8, 2, 28, 32, 32},
    {22, 0, 7, 0, 12, 13, 12, 12, 12, 10, 2, 28, 4, 30},
    {23, 5, 8, 14, 8, 8, 8, 8, 8, 8, 23, 0, 32, 32},
    {24, 5, 1, 0, 12, 12, 13, 12, 10, 12, 23, 0, 22, 6},
    {25, 6, 8, 14, 8, 8, 8, 8, 8, 8, 24, 28, 32, 32},
    {26, 6, 3, 0, 12, 12, 12, 13, 12, 10, 24, 28, 25, 12},
    {27, 4, 8, 14, 8, 8, 8, 8, 8, 8, 16, 8, 32, 32},
    {28, 4, 2, 0, 8, 8, 12, 11, 8, 13, 16, 8, 17, 10},
    {29, 7, 6, 0, 13, 13, 13, 13, 13, 12, 29, 10, 3, 25},
    {30, 7, 4, 0, 10, 10, 15, 14, 14, 14, 29, 17, 12, 0},
    {31, 7, 5, 0, 15, 15, 10, 10, 14, 14, 31, 23, 15, 27},
};

const char creature0[] PROGMEM = "SquibbleSnail";
const char creature1[] PROGMEM = "SquableSnail";
const char creature2[] PROGMEM = "ScrambleSnail";
const char creature3[] PROGMEM = "SkitterCrab";
const char creature4[] PROGMEM = "ScatterCrab";
const char creature5[] PROGMEM = "ShatterCrab";
const char creature6[] PROGMEM = "squid";
const char creature7[] PROGMEM = "bigsquid";
const char creature8[] PROGMEM = "BiggestSquid";
const char creature9[] PROGMEM = "bell";
const char creature10[] PROGMEM = "spark";
const char creature11[] PROGMEM = "cuircuit";
const char creature12[] PROGMEM = "hedge";
const char creature13[] PROGMEM = "cloud";
const char creature14[] PROGMEM = "rock";
const char creature15[] PROGMEM = "wiggleworm";
const char creature16[] PROGMEM = "waggleworm";
const char creature17[] PROGMEM = "skimskate";
const char creature18[] PROGMEM = "skimray";
const char creature19[] PROGMEM = "billow";
const char creature20[] PROGMEM = "howl";
const char creature21[] PROGMEM = "item";
const char creature22[] PROGMEM = "item2";
const char creature23[] PROGMEM = "zip";
const char creature24[] PROGMEM = "zap";
const char creature25[] PROGMEM = "suculent";
const char creature26[] PROGMEM = "cactus";
const char creature27[] PROGMEM = "flickerfly";
const char creature28[] PROGMEM = "flitfly";
const char creature29[] PROGMEM = "dragon?";
const char creature30[] PROGMEM = "skull";
const char creature31[] PROGMEM = "ardu";

const char *const creatureNames[] PROGMEM = {
    creature0,  creature1,  creature2,  creature3,  creature4,  creature5,
    creature6,  creature7,  creature8,  creature9,  creature10, creature11,
    creature12, creature13, creature14, creature15, creature16, creature17,
    creature18, creature19, creature20, creature21, creature22, creature23,
    creature24, creature25, creature26, creature27, creature28, creature29,
    creature30, creature31};