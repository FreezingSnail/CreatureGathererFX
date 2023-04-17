#pragma once
#include <avr/pgmspace.h> 
#include "../creature/Creature.hpp"
#include "../lib/Moves.hpp"


// going to need to embed these into progmem at somepoint
const PROGMEM CreatureSeed_t CreatureData[32] = { 
 { 0b00000000, 0b0001000100010001, movelistDebug, 0b00000000010001000111 }, //player debug creature
 { 0b00000000, 0b0001000100010001, movelistDebug, 0b00000000010001000111 }, //opponent debug creature
 { 0b00000000, 0b1111000010000010, 0b00010101111100000000000000000000 },
 { 0b00000001, 0b1111100000100010, 0b00010101111100000001001000001111 },
};
