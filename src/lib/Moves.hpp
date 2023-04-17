#pragma once
#include <stdint.h>
#include <avr/pgmspace.h> 

//move 
// 00    000    000    
// id#   type   power   


/******************
 * Array moves[move id]
 * 
 * Array Creature moves[Creature id][move list]
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * ***************/

//gana need 32 of these move lists
const PROGMEM uint32_t movelistDebug = 0b1111111111111111111111111111111;
const PROGMEM uint32_t movelist0 = 0b00000000000000000000000000000000;
const PROGMEM uint32_t movelist1 = 0b00000000000000000000000000000000;
const PROGMEM uint32_t movelist2 = 0b00000000000000000000000000000000;
const PROGMEM uint32_t movelist3 = 0b00000000000000000000000000000000;
const PROGMEM uint32_t movelist4 = 0b00000000000000000000000000000000;
const PROGMEM uint32_t movelist5 = 0b00000000000000000000000000000000;
const PROGMEM uint32_t movelist6 = 0b00000000000000000000000000000000;
const PROGMEM uint32_t movelist7 = 0b00000000000000000000000000000000;
const PROGMEM uint32_t movelist8 = 0b00000000000000000000000000000000;
const PROGMEM uint32_t movelist9 = 0b00000000000000000000000000000000;
const PROGMEM uint32_t movelist10 = 0b00000000000000000000000000000000;
const PROGMEM uint32_t movelist11 = 0b00000000000000000000000000000000;
const PROGMEM uint32_t movelist12 = 0b00000000000000000000000000000000;
const PROGMEM uint32_t movelist13 = 0b00000000000000000000000000000000;
const PROGMEM uint32_t movelist14 = 0b00000000000000000000000000000000;
const PROGMEM uint32_t movelist15 = 0b00000000000000000000000000000000;
const PROGMEM uint32_t movelist16 = 0b00000000000000000000000000000000;
const PROGMEM uint32_t movelist17 = 0b00000000000000000000000000000000;
const PROGMEM uint32_t movelist18 = 0b00000000000000000000000000000000;
const PROGMEM uint32_t movelist19 = 0b00000000000000000000000000000000;
const PROGMEM uint32_t movelist20 = 0b00000000000000000000000000000000;
const PROGMEM uint32_t movelist21 = 0b00000000000000000000000000000000;
const PROGMEM uint32_t movelist22 = 0b00000000000000000000000000000000;
const PROGMEM uint32_t movelist23 = 0b00000000000000000000000000000000;
const PROGMEM uint32_t movelist24 = 0b00000000000000000000000000000000;
const PROGMEM uint32_t movelist25 = 0b00000000000000000000000000000000;
const PROGMEM uint32_t movelist26 = 0b00000000000000000000000000000000;
const PROGMEM uint32_t movelist27 = 0b00000000000000000000000000000000;
const PROGMEM uint32_t movelist28 = 0b00000000000000000000000000000000;
const PROGMEM uint32_t movelist29 = 0b00000000000000000000000000000000;
const PROGMEM uint32_t movelist30 = 0b00000000000000000000000000000000;
const PROGMEM uint32_t movelist31 = 0b00000000000000000000000000000000;

//Should prob make a function to generate this instead of saving them all
//Plain, 	Water, 	Wind, 	Earth, 	Fire, 	Lightning, 	Plant, 	Elder

///////////////////////////////////////////////////////
//              Moves                               //
//////////////////////////////////////////////////////

// 32 moves, 8 types, 4 per type

// Type 000 0 //plain

const PROGMEM uint8_t m00 = 0b00000001;
//
const PROGMEM uint8_t m01 = 0b01000010;
//
const PROGMEM uint8_t m02 = 0b10000100;
//
const PROGMEM uint8_t m03 = 0b11000101;
//

// Type 001 1 //water

const PROGMEM uint8_t m04 = 0b00001001;
//
const PROGMEM uint8_t m05 = 0b01001010;
//
const PROGMEM uint8_t m06 = 0b10001100;
//
const PROGMEM uint8_t m07 = 0b11001101;
//

// Type 010 2 //wind

const PROGMEM uint8_t m08 = 0b00001001;
//
const PROGMEM uint8_t m09 = 0b01001010;
//
const PROGMEM uint8_t m10 = 0b10001100;
//
const PROGMEM uint8_t m11 = 0b11001101;
//
// Type 011 3 //earth

const PROGMEM uint8_t m12 = 0b00001001;
//
const PROGMEM uint8_t m13 = 0b01001010;
//
const PROGMEM uint8_t m14 = 0b10001100;
//
const PROGMEM uint8_t m15 = 0b11001101;
//
// Type 100 4 //fire

const PROGMEM uint8_t m16 = 0b00001001;
//
const PROGMEM uint8_t m17 = 0b01001010;
//
const PROGMEM uint8_t m18 = 0b10001100;
//
const PROGMEM uint8_t m19 = 0b11001101;
//
// Type 101 5 //lightning

const PROGMEM uint8_t m20 = 0b00001001;
//
const PROGMEM uint8_t m21 = 0b01001010;
//
const PROGMEM uint8_t m22 = 0b10001100;
//
const PROGMEM uint8_t m23 = 0b11001101;
//
// Type 110 6 //plant

const PROGMEM uint8_t m24 = 0b00001001;
//
const PROGMEM uint8_t m25 = 0b01001010;
//
const PROGMEM uint8_t m26 = 0b10001100;
//
const PROGMEM uint8_t m27 = 0b11001101;
//
// Type 111 7 //elder

const PROGMEM uint8_t m28 = 0b00001001;
//
const PROGMEM uint8_t m29 = 0b01001010;
//
const PROGMEM uint8_t m30 = 0b10001100;
//
const PROGMEM uint8_t m31 = 0b11001101;
//



