#pragma once
#include <stdint.h>
#include "Effect.hpp"
#ifdef __AVR__
#include <avr/pgmspace.h>
#define TYPE_TABLE_STORAGE PROGMEM
#else
#define TYPE_TABLE_STORAGE
#endif

// represented as a nibble 0-15 range
enum class Type {
    SPIRIT,
    WATER,
    WIND,
    EARTH,
    FIRE,
    LIGHTNING,
    PLANT,
    ELDER,

    STATUS,

    NONE = 15,
};

constexpr const uint8_t TypeCount = 9;

class DualType {
  private:
    uint8_t value;

    static const uint8_t Type1Mask = 0b11110000;
    static const uint8_t Type2Mask = 0b00001111;
    static const uint8_t Type1Shift = 4;
    static const uint8_t Type2Shift = 0;

    constexpr uint8_t packTypes(Type type1, Type type2) {
        return ((static_cast<uint8_t>(type1)) << Type1Shift) | ((static_cast<uint8_t>(type2) & Type2Mask));
    }

  public:
    constexpr DualType() : value(packTypes(Type::NONE, Type::NONE)) {
    }
    constexpr DualType(Type type) : value(packTypes(type, Type::NONE)) {
    }
    constexpr DualType(Type type1, Type type2) : value(packTypes(type1, type2)) {
    }

    constexpr Type getType1(void) const {
        return static_cast<Type>((this->value >> Type1Shift));
    }

    constexpr Type getType2(void) const {
        return static_cast<Type>((this->value & Type2Mask) >> Type2Shift);
    }

    constexpr bool hasType(Type type) const {
        return ((this->getType1() == type) || (this->getType2() == type));
    }
};

enum class Modifier : uint8_t {
    None,
    Quarter,
    Half,
    Same,
    Double,
    Quadruple,
};

static uint16_t applyMod(uint16_t value, Modifier modifier) {
    switch (modifier) {
    case Modifier::Same:
        return value;
    case Modifier::Half:
        return value >> 1;
    case Modifier::Double:
        return value << 1;
    case Modifier::Quarter:
        return value >> 2;
    case Modifier::Quadruple:
        return value << 2;
    default:
        return value;
    }
}

extern const Modifier typeTable[TypeCount][TypeCount] TYPE_TABLE_STORAGE;

static Modifier getModifier(Type attackType, Type defendingType) {
    if (defendingType == Type::NONE) {
        return Modifier::Same;
    }
    if (attackType == Type::NONE) return Modifier::None;
#ifdef __AVR__
    return static_cast<Modifier>(pgm_read_byte(
        &typeTable[static_cast<uint8_t>(attackType)][static_cast<uint8_t>(defendingType)]));
#else
    return typeTable[static_cast<uint8_t>(attackType)][static_cast<uint8_t>(defendingType)];
#endif
}

static Modifier combineModifier(Modifier a, Modifier b) {
    if (a == Modifier::None || b == Modifier::None) return Modifier::None;
    int8_t exponent = static_cast<int8_t>(a) + static_cast<int8_t>(b) - 6;
    if (exponent < -2) exponent = -2;
    if (exponent > 2) exponent = 2;
    return static_cast<Modifier>(exponent + 3);
}

static Modifier getModifier(Type attackType, DualType defendingType) {
    Modifier m1 = getModifier(attackType, defendingType.getType1());
    Modifier m2 = getModifier(attackType, defendingType.getType2());
    return combineModifier(m1, m2);
}

static uint16_t applyModifier(uint16_t baseValue, Type attackType, DualType defendingType, Modifier effectMod) {
    const Modifier mod = getModifier(attackType, defendingType);
    baseValue = applyMod(baseValue, combineModifier(mod, effectMod));
    return baseValue;
}

// Elemental suppression/boost effects follow the eight elemental types in order.
static Modifier typeEffectModifier(Effect effect, DualType type) {
    const uint8_t id = static_cast<uint8_t>(effect);
    if (id > static_cast<uint8_t>(Effect::EVOLVD) ||
        !type.hasType(static_cast<Type>(id & 7u))) {
        return Modifier::Same;
    }
    return id < static_cast<uint8_t>(Effect::ENLTND)
        ? Modifier::Half : Modifier::Double;
}

static Modifier typeEffectPairModifier(Effect first, Effect second,
                                      DualType type) {
    return combineModifier(typeEffectModifier(first, type),
                           typeEffectModifier(second, type));
}

static Modifier inverseModifier(Modifier mod) {
    const uint8_t value = static_cast<uint8_t>(mod);
    return value >= static_cast<uint8_t>(Modifier::Quarter) &&
           value <= static_cast<uint8_t>(Modifier::Quadruple)
        ? static_cast<Modifier>(6u - value) : Modifier::None;
}
