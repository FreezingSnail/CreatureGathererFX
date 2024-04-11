#pragma once
#include <stdint.h>

enum class Accuracy { HUNDRED, NINETY, EIGHTY, SEVENTY };

enum class Effect { NONE, EGOED, DRENCHED, BUFFETED, STUMBLED, BURNED, SHOCKED, ENTANGLED, CURSED };

struct MoveBitSet {
    uint8_t type;       // 3 bit
    uint8_t power;      // 5 bit
    uint8_t physical;   // 1 bit
    uint8_t accuracy;   // 2 bit
    uint8_t effect;     // 5 bit (32 potential effects)
};

class Move {
  private:
    // 000 0000 0
    static const uint16_t TypeMask = 0b1110000000000000;
    static const uint16_t PowerMask = 0b0001111100000000;
    static const uint16_t PhysMask = 0b0000000010000000;
    static const uint16_t AccMask = 0b0000000001100000;
    static const uint16_t EffectMask = 0b0000000000011111;
    static const uint8_t TypeShift = 13;
    static const uint8_t PowerShift = 8;
    static const uint8_t PhysShift = 7;
    static const uint8_t AccShift = 5;

  public:
    uint16_t move;

    constexpr uint16_t packMove(MoveBitSet move) {
        return (move.type << TypeShift) | (move.power << PowerShift) | (move.physical << PhysShift) | (move.accuracy << AccShift) |
               (move.effect);
    }

    constexpr Move() : move(0) {
    }

    constexpr Move(MoveBitSet movePack) : move(packMove(movePack)) {
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
        return Effect((this->move & EffectMask));
    }
};