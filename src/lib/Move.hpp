#pragma once
#include <stdint.h>
#include <avr/pgmspace.h> 
#include "Moves.hpp"

#ifdef CLI
	#include <iostream>
	#include <bitset>
#endif

static const uint8_t* moveList[32] =	{ &m00, &m01, &m02, &m03, &m04, &m05, &m06, &m07, &m08, &m09, 
										&m10, &m11, &m12, &m13, &m14, &m15, &m16, &m17, &m18, &m19, 
										&m20, &m21, &m22, &m23, &m24, &m25, &m26, &m27, &m28, &m29, 
										&m31, &m31 };

const char move0[] PROGMEM = "move1";
const char move1[] PROGMEM = "move2";
const char move2[] PROGMEM = "move3";
const char move3[] PROGMEM = "move4";
const char move4[] PROGMEM = "move5";
const char move5[] PROGMEM = "move6";
const char move6[] PROGMEM = "move7";
const char move7[] PROGMEM = "move8";
const char move8[] PROGMEM = "move9";
const char move9[] PROGMEM = "move10";
const char move10[] PROGMEM = "move11";
const char move11[] PROGMEM = "move12";
const char move12[] PROGMEM = "move13";
const char move13[] PROGMEM = "move14";
const char move14[] PROGMEM = "move15";
const char move15[] PROGMEM = "move16";
const char move16[] PROGMEM = "move17";
const char move17[] PROGMEM = "move18";
const char move18[] PROGMEM = "move19";
const char move19[] PROGMEM = "move20";
const char move20[] PROGMEM = "move21";
const char move21[] PROGMEM = "move22";
const char move22[] PROGMEM = "move23";
const char move23[] PROGMEM = "move24";
const char move24[] PROGMEM = "move25";
const char move25[] PROGMEM = "move26";
const char move26[] PROGMEM = "move27";
const char move27[] PROGMEM = "move28";
const char move28[] PROGMEM = "move29";
const char move29[] PROGMEM = "move30";
const char move30[] PROGMEM = "move31";
const char move31[] PROGMEM = "move32";

const char* const moveNames[] PROGMEM = {move1, move2, move3, move4,  move5, 
					move6, move7, move8, move9, move10, move11, move12, move13, move14, 
					move15, move16, move17, move18, move19, move20, move21, move22, move23, 
					move24, move25, move26, move27, move28, move29, move30, move31};
//move 
// 00    000    000    
// id#   type   power 
static uint8_t getMovePower(uint8_t move){
	return move & 0b00000111;
}

static uint8_t getMoveType(uint8_t move){

	return ((move & 0b00111000) >> 3);
}

// Each type has 4 moves 
// The posistion of these 4 is stored in the first 2 bits
// The index of the move globally is a function of its type and id
static uint8_t getMoveID(uint8_t move){
	return (((4) * ((move >> 3) & 0b111)) + (move >> 6));
}

static uint8_t getMove(uint8_t index) {
	return pgm_read_word_near(moveList[index]);
}
