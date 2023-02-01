#pragma once
#include "Moves.h"


static const uint8_t moveList[8] =	{ m00, m01, m02, m03, m04, m05, m06, m07 /*, m01, m01, m01, m01, m01, m01, m01, m01, m01, m01, m01, m01, m01, m01, m01, m01, m01, m01, m01, m01, m01, m01, m01, m01 */};

static uint8_t getMoveID(uint8_t move){
	//move 
	// 00    000    000    
	// id#   type   power 
	uint8_t x =(((4) * ((move >> 3) & 0b111) + (move >> 6)));

	#ifdef DEBUG
		//std::cout << "move id: " << x << std::endl;
		std::cout << "bitset: " << std::bitset<8>((move >> 6)) << std::endl;
		std::cout << "num: " << (move >> 6) << std::endl;
	#endif

	return x;
}


static uint8_t getMoveType(uint8_t move){

	return (moveList[move]>>3)& 0b111;
}