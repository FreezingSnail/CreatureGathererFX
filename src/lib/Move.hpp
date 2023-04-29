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

const char move1[] PROGMEM = "move1";
const char move2[] PROGMEM = "move2";
const char move3[] PROGMEM = "move3";
const char move4[] PROGMEM = "move4";

const char* const moveNames[] PROGMEM = {move1, move2, move3, move4};
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
	return (((4) * ((move >> 3) & 0b111) + (move >> 6)));
}

static uint8_t getMove(uint8_t index) {
	return pgm_read_word_near(moveList[index]);
}
