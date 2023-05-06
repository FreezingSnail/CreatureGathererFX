#pragma once
#include <avr/pgmspace.h> 


//todo research huffman encoding to squash these in mem
typedef struct CreatureD{
    unsigned char id;
	unsigned char type1 ;
	unsigned char type2 ;
	unsigned char hpSeed;
	unsigned char atkSeed;
	unsigned char defSeed;	
	unsigned char spcAtkSeed;	
	unsigned char spcDefSeed;	
	unsigned char spdSeed;
	unsigned char evoLevel  ;
	unsigned char creatureID;
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
{ 3, 3, 8, 8, 3, 3, 3, 3, 3, 3, 0, 0, 0, 0 },
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
{ 15, 1, 8, 0, 8, 8, 8, 8, 8, 8, 0, 0, 0, 0 },
{ 16, 3, 8, 14, 12, 12, 12, 12, 12, 12, 0, 0, 0, 0 },
{ 17, 3, 4, 0, 8, 8, 8, 8, 8, 8, 0, 0, 0, 0 },
{ 18, 1, 8, 14, 8, 8, 8, 8, 8, 8, 0, 0, 0, 0 },
{ 19, 1, 6, 0, 12, 12, 12, 12, 12, 12, 0, 0, 0, 0 },
{ 20, 2, 8, 14, 8, 8, 8, 8, 8, 8, 0, 0, 0, 0 },
{ 21, 2, 5, 0, 12, 12, 12, 12, 12, 12, 0, 0, 0, 0 },
{ 22, 0, 8, 14, 8, 8, 8, 8, 8, 8, 0, 0, 0, 0 },
{ 23, 0, 7, 0, 12, 12, 12, 12, 12, 12, 0, 0, 0, 0 },
{ 24, 5, 8, 14, 8, 8, 8, 8, 8, 8, 0, 0, 0, 0 },
{ 25, 5, 1, 0, 12, 12, 12, 12, 12, 12, 0, 0, 0, 0 },
{ 26, 6, 8, 14, 8, 8, 8, 8, 8, 8, 0, 0, 0, 0 },
{ 27, 6, 3, 0, 12, 12, 12, 12, 12, 12, 0, 0, 0, 0 },
{ 28, 4, 8, 14, 8, 8, 8, 8, 8, 8, 0, 0, 0, 0 },
{ 29, 4, 2, 0, 12, 12, 12, 12, 12, 12, 0, 0, 0, 0 },
{ 30, 7, 6, 0, 8, 8, 8, 8, 8, 8, 0, 0, 0, 0 },
{ 31, 7, 4, 0, 10, 10, 15, 14, 14, 14, 0, 0, 0, 0 },
};
