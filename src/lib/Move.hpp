#pragma once
#include "Type.hpp"
#include <avr/pgmspace.h>
#include <stdint.h>

#ifdef CLI
#include <bitset>
#include <iostream>
#endif

// move
//  00    000    000
//  id#   type   power

struct MoveBitSet {
  uint8_t type : 3;
  uint8_t power : 4;
  uint8_t physical : 1;
};

const MoveBitSet movePack[32] PROGMEM = {
    {0, 10, 0}, {0, 5, 1},  {0, 7, 1},  {0, 10, 1}, {1, 4, 0},  {1, 7, 0},
    {1, 10, 0}, {1, 7, 1},  {2, 5, 0},  {2, 10, 0}, {2, 10, 1}, {2, 7, 1},
    {3, 7, 0},  {3, 5, 1},  {3, 7, 1},  {3, 10, 1}, {4, 8, 0},  {4, 12, 0},
    {4, 12, 1}, {4, 6, 1},  {5, 7, 0},  {5, 12, 0}, {5, 12, 1}, {5, 7, 1},
    {6, 6, 0},  {6, 12, 0}, {6, 12, 1}, {6, 8, 1},  {7, 8, 0},  {7, 12, 0},
    {7, 10, 1}, {7, 14, 1},
};

const char move0[] PROGMEM = "smite";
const char move1[] PROGMEM = "thought";
const char move2[] PROGMEM = "wisper";
const char move3[] PROGMEM = "blast";
const char move4[] PROGMEM = "splash";
const char move5[] PROGMEM = "squirt";
const char move6[] PROGMEM = "soak";
const char move7[] PROGMEM = "current";
const char move8[] PROGMEM = "breeze";
const char move9[] PROGMEM = "bellow";
const char move10[] PROGMEM = "torrent";
const char move11[] PROGMEM = "blow";
const char move12[] PROGMEM = "dirtbeam";
const char move13[] PROGMEM = "slipfall";
const char move14[] PROGMEM = "dirtfall";
const char move15[] PROGMEM = "shatter";
const char move16[] PROGMEM = "burn";
const char move17[] PROGMEM = "melt";
const char move18[] PROGMEM = "torch";
const char move19[] PROGMEM = "stoke";
const char move20[] PROGMEM = "bolt";
const char move21[] PROGMEM = "plasma";
const char move22[] PROGMEM = "coil";
const char move23[] PROGMEM = "zap";
const char move24[] PROGMEM = "pollen";
const char move25[] PROGMEM = "pollenbeam";
const char move26[] PROGMEM = "seedfall";
const char move27[] PROGMEM = "root";
const char move28[] PROGMEM = "beam";
const char move29[] PROGMEM = "elderBeam";
const char move30[] PROGMEM = "slam";
const char move31[] PROGMEM = "elderSlam";

const char *const moveNames[32] PROGMEM = {
    move0,  move1,  move2,  move3,  move4,  move5,  move6,  move7,
    move8,  move9,  move10, move11, move12, move13, move14, move15,
    move16, move17, move18, move19, move20, move21, move22, move23,
    move24, move25, move26, move27, move28, move29, move30, move31};

static MoveBitSet getMovePack(uint8_t index) {
  MoveBitSet move;
  memcpy_P(&move, &movePack[index], sizeof(MoveBitSet));
  return move;
}

class Move {
private:
  uint8_t move;

  // 000 0000 0
  static const uint8_t TypeMask = 0b11100000;
  static const uint8_t PowerMask = 0b00011110;
  static const uint8_t PhysMask = 0b00000001;
  static const uint8_t TypeShift = 5;
  static const uint8_t PowerShift = 1;

  constexpr uint8_t packMove(MoveBitSet move) {
    return (move.type << TypeShift) | (move.power << PowerShift) |
           move.physical;
  }

public:
  constexpr Move() : move(0) {}

  constexpr Move(MoveBitSet movePack) : move(packMove(movePack)) {}

  constexpr uint8_t getMovePower() { return (this->move & PowerMask) >> 1; }

  constexpr uint8_t getMoveType() { return ((this->move & TypeMask) >> 5); }

  constexpr bool isPhysical() { return (this->move & PhysMask); }
};

// gana need 32 of these move lists, for each creature
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