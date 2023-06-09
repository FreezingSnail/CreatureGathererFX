#pragma once
#include <avr/pgmspace.h> 
#include "../opponent/OpponentSeed.hpp"

/* monId lvl mov1 mov2 mov3 mov4 x3, 3 32bit ints

    3 32 bit ints to store opponent seeds
*/
const PROGMEM OpponentSeed_t op1 = {  0b00000010000100001000010000100001, 
                                       0b00000010000100001000010000100001, 
                                    0b00000010000100001000010000100001 };

const PROGMEM OpponentSeed_t opts[] = {{  0b00000010000100001000010000100001, 
                                       0b00000010000100001000010000100001, 
                                    0b00000010000100001000010000100001 }};