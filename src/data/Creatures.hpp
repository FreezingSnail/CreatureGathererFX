#pragma once
#include <avr/pgmspace.h> 


//todo research huffman encoding to squash these in mem
typedef struct CreatureD{
    unsigned char id;
	unsigned char type1 ;
	unsigned char type2 ;
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
}CreatureData_t;

// going to need to embed these into progmem at somepoint
// each instance wastes 4 bits

const CreatureData_t CreatureData[32] PROGMEM = { 
{ 0, 2, 8, 8, 3, 3, 3, 3, 3, 3, 0, 0, 0, 0 },
{ 1, 2, 8, 16, 8, 8, 8, 8, 8, 8, 0, 0, 0, 0 },
{ 2, 2, 6, 24, 12, 12, 12, 12, 12, 12, 0, 0, 0, 0 },
{ 3, 3, 8, 8, 3, 3, 3, 3, 3, 3, 5, 7, 9, 11 },
{ 4, 3, 8, 16, 8, 8, 8, 8, 8, 8, 0, 0, 0, 0 },
{ 5, 3, 5, 24, 12, 12, 12, 12, 12, 12, 0, 0, 0, 0 },
{ 6, 1, 8, 8, 3, 3, 3, 3, 3, 3, 0, 0, 0, 0 },
{ 7, 1, 8, 16, 8, 8, 8, 8, 8, 8, 0, 0, 0, 0 },
{ 8, 1, 4, 24, 12, 12, 12, 12, 12, 12, 0, 0, 0, 0 },
{ 9, 0, 8, 0, 3, 3, 3, 3, 3, 3, 0, 0, 0, 0 },
{ 10, 4, 8, 0, 3, 3, 3, 3, 3, 3, 0, 0, 0, 0 },
{ 11, 5, 8, 0, 3, 3, 3, 3, 3, 3, 0, 0, 0, 0 },
{ 12, 6, 8, 0, 3, 3, 3, 3, 3, 3, 0, 0, 0, 0 },
{ 13, 2, 8, 0, 8, 8, 8, 8, 8, 8, 0, 0, 0, 0 },
{ 14, 3, 8, 0, 8, 8, 8, 8, 8, 8, 0, 0, 0, 0 },
{ 15, 3, 8, 14, 12, 12, 12, 12, 12, 12, 0, 0, 0, 0 },
{ 16, 3, 4, 0, 8, 8, 8, 8, 8, 8, 0, 0, 0, 0 },
{ 17, 1, 8, 14, 8, 8, 8, 8, 8, 8, 0, 0, 0, 0 },
{ 18, 1, 6, 0, 12, 12, 12, 12, 12, 12, 0, 0, 0, 0 },
{ 19, 2, 8, 14, 8, 8, 8, 8, 8, 8, 0, 0, 0, 0 },
{ 20, 2, 5, 0, 12, 12, 12, 12, 12, 12, 0, 0, 0, 0 },
{ 21, 0, 8, 14, 8, 8, 8, 8, 8, 8, 0, 0, 0, 0 },
{ 22, 0, 7, 0, 12, 12, 12, 12, 12, 12, 0, 0, 0, 0 },
{ 23, 5, 8, 14, 8, 8, 8, 8, 8, 8, 0, 0, 0, 0 },
{ 24, 5, 1, 0, 12, 12, 12, 12, 12, 12, 0, 0, 0, 0 },
{ 25, 6, 8, 14, 8, 8, 8, 8, 8, 8, 0, 0, 0, 0 },
{ 26, 6, 3, 0, 12, 12, 12, 12, 12, 12, 0, 0, 0, 0 },
{ 27, 4, 8, 14, 8, 8, 8, 8, 8, 8, 0, 0, 0, 0 },
{ 28, 4, 2, 0, 12, 12, 12, 12, 12, 12, 0, 0, 0, 0 },
{ 29, 7, 6, 0, 8, 8, 8, 8, 8, 8, 0, 0, 0, 0 },
{ 30, 7, 4, 0, 10, 10, 15, 14, 14, 14, 0, 0, 0, 0 },
{ 31, 7, 5, 0, 15, 15, 10, 10, 14, 14, 0, 0, 0, 0 },
};

const char creature0[]  PROGMEM= "SquibbleSnail";
const char creature1[]  PROGMEM= "SquableSnail";
const char creature2[]  PROGMEM= "ScrambleSnail";
const char creature3[]  PROGMEM= "SkitterCrab";
const char creature4[]  PROGMEM= "ScatterCrab";
const char creature5[]  PROGMEM= "ShatterCrab";
const char creature6[]  PROGMEM= "squid";
const char creature7[]  PROGMEM= "bigsquid";
const char creature8[]  PROGMEM= "BiggestSquid";
const char creature9[]  PROGMEM= "bell";
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

const char * const creatureNames[] PROGMEM = { creature0, creature1, creature2, creature3, creature4, 
							creature5, creature6, creature7, creature8, creature9, creature10, creature11, creature12, 
							creature13, creature14, creature15, creature16, creature17, creature18, creature19, creature20, 
							creature21, creature22, creature23, creature24, creature25, creature26, creature27, creature28, 
							creature29, creature30, creature31
							};