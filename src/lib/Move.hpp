#pragma once
#include "../fxdata.h"
#include "Type.hpp"
#include <ArduboyFX.h>
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
    uint8_t type;
    uint8_t power;
    uint8_t physical;
};

static MoveBitSet getMovePack(uint8_t index) {
    MoveBitSet move;
    uint24_t rowAddress = MoveData::movePack + sizeof(MoveBitSet) * index;
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
        return (move.type << TypeShift) | (move.power << PowerShift) | move.physical;
    }

  public:
    constexpr Move() : move(0) {
    }

    constexpr Move(MoveBitSet movePack) : move(packMove(movePack)) {
    }

    constexpr uint8_t getMovePower() {
        return (this->move & PowerMask) >> 1;
    }

    constexpr uint8_t getMoveType() {
        return ((this->move & TypeMask) >> 5);
    }

    constexpr bool isPhysical() {
        return (this->move & PhysMask);
    }
};