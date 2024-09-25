#pragma once
#include <stdint.h>
#include "Effect.hpp"

#define dbf __attribute__((optimize("-O0")))

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

// TODO: Move to FX data
const Modifier typeTable[TypeCount][TypeCount] = {
    // Spirit
    {
        Modifier::Same,   // Spirit
        Modifier::Same,   // Water
        Modifier::Same,   // Wind
        Modifier::Same,   // Earth
        Modifier::Same,   // Fire
        Modifier::Same,   // Lightning
        Modifier::Same,   // Plant
        Modifier::None,   // Elder

    },
    // Water
    {
        Modifier::Same,     // Spirit
        Modifier::Same,     // Water
        Modifier::Double,   // Wind
        Modifier::Half,     // Earth
        Modifier::Double,   // Fire
        Modifier::Same,     // Lightning
        Modifier::Half,     // Plant
        Modifier::Half,     // Elder
    },
    // wind
    {
        Modifier::Same,     // Spirit
        Modifier::Same,     // Water
        Modifier::Same,     // Wind
        Modifier::Double,   // Earth
        Modifier::Half,     // Fire
        Modifier::Half,     // Lightning
        Modifier::Double,   // Plant
        Modifier::Half,     // Elder
    },
    // Earth
    {
        Modifier::Same,     // Spirit
        Modifier::Double,   // Water
        Modifier::Half,     // Wind
        Modifier::Same,     // Earth
        Modifier::Same,     // Fire
        Modifier::Double,   // Lightning
        Modifier::Half,     // Plant
        Modifier::Half,     // Elder
    },
    // Fire
    {
        Modifier::Same,     // Spirit
        Modifier::None,     // Water
        Modifier::Double,   // Wind
        Modifier::Half,     // Earth
        Modifier::Same,     // Fire
        Modifier::Double,   // Lightning
        Modifier::Double,   // Plant
        Modifier::Same,     // Elder

    },
    // Lightning
    {
        Modifier::Same,     // Spirit
        Modifier::Double,   // Water
        Modifier::Double,   // Wind
        Modifier::Half,     // Earth
        Modifier::Same,     // Fire
        Modifier::Double,   // Lightning
        Modifier::Double,   // Plant
        Modifier::Same,     // Elder

    },
    // Plant
    {
        Modifier::Same,     // Spirit
        Modifier::None,     // Water
        Modifier::Double,   // Wind
        Modifier::Half,     // Earth
        Modifier::Same,     // Fire
        Modifier::Double,   // Lightning
        Modifier::Double,   // Plant
        Modifier::Same,     // Elder

    },
    // Elder
    {
        Modifier::Same,     // Spirit
        Modifier::None,     // Water
        Modifier::Double,   // Wind
        Modifier::Half,     // Earth
        Modifier::Same,     // Fire
        Modifier::Double,   // Lightning
        Modifier::Double,   // Plant
        Modifier::Same,     // Elder

    },

};

static Modifier getModifier(Type attackType, Type defendingType) {
    if (defendingType == Type::NONE) {
        return Modifier::Same;
    }
    return (attackType == Type::NONE) ? Modifier::None : static_cast<Modifier>((typeTable[static_cast<uint8_t>(attackType)][static_cast<uint8_t>(defendingType)]));
}

static Modifier combineModifier(Modifier a, Modifier b) {
    if (a == Modifier::None || b == Modifier::None) {
        return Modifier::None;
    }
    switch (a) {
    case Modifier::Same:
        return b;
    case Modifier::Half:
        switch (b) {
        case Modifier::Double:
            return Modifier::Same;
        case Modifier::Half:
            return Modifier::Quarter;
        default:
            return Modifier::Half;
        }
    case Modifier::Double:
        switch (b) {
        case Modifier::Double:
            return Modifier::Quadruple;
        case Modifier::Half:
            return Modifier::Same;
        default:
            return Modifier::Double;
        }
    }

    return Modifier::None;
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

static Modifier typeEffectModifier(Effect effect, DualType type) {
    DualType modType = DualType(Type::NONE, Type::NONE);
    bool increase;
    switch (effect) {
    case Effect::NONE:
        return Modifier::Same;
    case Effect::DPRSD:
        increase = false;
        modType = DualType(Type::SPIRIT, Type::NONE);
        break;
    case Effect::SOAKED:
        increase = false;
        modType = DualType(Type::WATER, Type::NONE);
        break;
    case Effect::BUFTD:
        increase = false;
        modType = DualType(Type::WIND, Type::NONE);
        break;
    case Effect::SOILED:
        increase = false;
        modType = DualType(Type::EARTH, Type::NONE);
        break;
    case Effect::SCRCHD:
        increase = false;
        modType = DualType(Type::FIRE, Type::NONE);
        break;
    case Effect::ZAPPED:
        increase = false;
        modType = DualType(Type::LIGHTNING, Type::NONE);
        break;
    case Effect::TANGLD:
        increase = false;
        modType = DualType(Type::PLANT, Type::NONE);
        break;
    case Effect::REDCD:
        increase = false;
        modType = DualType(Type::ELDER, Type::NONE);
        break;
    case Effect::ENLTND:
        increase = true;
        modType = DualType(Type::SPIRIT, Type::NONE);
        break;
    case Effect::DRNCHD:
        increase = true;
        modType = DualType(Type::WATER, Type::NONE);
        break;
    case Effect::AIRSWPT:
        increase = true;
        modType = DualType(Type::WIND, Type::NONE);
        break;
    case Effect::GRNDED:
        increase = true;
        modType = DualType(Type::EARTH, Type::NONE);
        break;
    case Effect::KINDLD:
        increase = true;
        modType = DualType(Type::FIRE, Type::NONE);
        break;
    case Effect::CHRGD:
        increase = true;
        modType = DualType(Type::LIGHTNING, Type::NONE);
        break;
    case Effect::ENRCHD:
        increase = true;
        modType = DualType(Type::PLANT, Type::NONE);
        break;
    case Effect::EVOLVD:
        increase = true;
        modType = DualType(Type::ELDER, Type::NONE);
        break;
    }

    if (!type.hasType(modType.getType1())) {
        return Modifier::Same;
    }
    if (increase) {
        return Modifier::Double;
    } else {
        return Modifier::Half;
    }
}

static Modifier inverseModifier(Modifier mod) {
    switch (mod) {
    case Modifier::Same:
        return Modifier::Same;
    case Modifier::Half:
        return Modifier::Double;
    case Modifier::Double:
        return Modifier::Half;
    case Modifier::Quarter:
        return Modifier::Quadruple;
    case Modifier::Quadruple:
        return Modifier::Quarter;
    default:
        return Modifier::None;
    }
}
