#pragma once
#include "Type.hpp"
#include <avr/pgmspace.h>
#include "../fxdata/fxdata.h"
#include <stdint.h>
#include <ArduboyFX.h>


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



static MoveBitSet getMovePack(uint8_t index) {
  MoveBitSet move;
  uint24_t rowAddress = FX::readIndexedUInt24(MoveData::movePack, index);
  FX::readDataObject(rowAddress, move);
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