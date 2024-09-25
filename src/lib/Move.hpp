#pragma once
#include <stdint.h>
#include "Effect.hpp"

enum class Accuracy {
    HUNDRED = 0,
    NINETY = 1,
    EIGHTY = 2,
    SEVENTY = 3
};

// TODO: redsign to get rid of this entirely
struct MoveBitSet {
    uint8_t type;       // 4 bit
    uint8_t power;      // 5 bit
    uint8_t physical;   // 1 bit
    uint8_t accuracy;   // 2 bit

    // TODO: mabye make this 2 effects per move
    uint8_t effects;   // 5 bit (32 potential effects)
};

class Move {
  private:
    static const uint16_t TypeMask = 0b1111000000000000;
    static const uint16_t PowerMask = 0b0000111110000000;
    static const uint16_t PhysMask = 0b00000000001000000;
    static const uint16_t AccMask = 0b0000000000110000;
    static const uint8_t TypeShift = 12;
    static const uint8_t PowerShift = 7;
    static const uint8_t PhysShift = 6;
    static const uint8_t AccShift = 4;

  public:
    uint16_t move;
    Effect effect1;
    Effect effect2;

    constexpr uint16_t packMove(MoveBitSet move) {
        return (move.type << TypeShift) | (move.power << PowerShift) | (move.physical << PhysShift) | (move.accuracy << AccShift);
    }

    constexpr Move() : move(0), effect1(Effect::NONE), effect2(Effect::NONE) {
    }

    constexpr Move(MoveBitSet movePack) : move(packMove(movePack)), effect1(Effect::NONE), effect2(Effect::NONE) {
    }

    constexpr Move(uint32_t buffer) : move(buffer >> 16), effect1(Effect((buffer >> 8) & 0b11111111)), effect2(Effect(buffer & 0b11111111)) {
    }
    constexpr uint8_t getMovePower() {
        return (this->move & PowerMask) >> PowerShift;
    }

    constexpr uint8_t getMoveType() {
        return (this->move & TypeMask) >> TypeShift;
    }

    constexpr bool isPhysical() {
        return (this->move & PhysMask) >> PhysShift;
    }

    constexpr Accuracy getMoveAccuracy() {
        return Accuracy((this->move & AccMask) >> AccShift);
    }

    constexpr Effect getMoveEffect() {
        return Effect((this->effect1));
    }
};
