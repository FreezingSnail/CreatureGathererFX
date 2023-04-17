#pragma once
#include <stdint.h>
#include <iostream>

typedef enum class Types{
	plain,
	water,
	wind,
	earth,
	fire,
	lightning,
	plant,
	elder,
}Type_t;

/*************
/	8x8 array  defense ->
/atk
/ |			  Plain, 	Water, 	Wind, 	Earth, 	Fire, 	Lightning, 	Plant, 	Elder
/ V		p 	  1			1		1		1		1		1			1		1	
/		
		Wa 	  1			.5		1		2		2		1			0		.5

		wi 	  1			1		1		.5		2		.5			1		1

		E     1			.5		0		.5		1		2			.5		1

		F     1			.5		1		.5		1		1			2		2

		L     1			2		2		0		1		1			.5		.5

		p     1			2		.5		2		.5		1			1		.5

		E     2			1		1		1		0		1			1		2
/
/
/
/		0	.5	1	2
/		0	1	2	3
/
/************/

// pack into pairs into 8bit ints
/*uint8_t typeTableb[8][8] = {
	{2,2,2,2,2,2,2,2},
	{2,2,2,4,4,2,0,1};
	{2,2,2,1,4,1,2,2},
	{2,1,0,1,2,4,1,2},
	{2,1,2,1,2,2,4,4},
	{2,4,4,0,2,2,1,1},
	{2,4,1,4,1,2,2,1},
	{4,2,2,2,0,2,2,4}
};
*/
static const uint8_t typeTable[8][2] = 
{   // 0 1 2 3     4 5 6 7 
	{0b10101010, 0b10101010},
	{0b10101011, 0b11100001},
	{0b10101001, 0b11011010},
	{0b10010001, 0b10110110},
	{0b10011001, 0b10101111},
	{0b10111100, 0b10100101}, 
	{0b10110111, 0b01101001},
	{0b11101010, 0b00101011}
};



static uint8_t getMatchupModifier(uint8_t AttackT, uint8_t defType){

	uint8_t mod = (uint8_t)(typeTable[AttackT][(defType/4)]);
    // std::cout << "attakt: " << unsigned(AttackT) << " deft: " << unsigned(defType) << std::endl;
    // std::cout << "mod: " << unsigned(mod) << " type table: " << unsigned(typeTable[AttackT][(defType/4)]) << std::endl;
	switch (defType%4){
		case 0:
			return (mod >> 6);
		case 1:
			return ((mod >> 4) & 0b11);
		case 2:
			return ((mod >> 2) & 0b11);
		case 3:
			return (mod & 0b11);
		default:
			return 5;
	};
}