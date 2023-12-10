#pragma once
#include <stdint.h>

const char spirit[] = "Spirit";
const char water[] = "Water";
const char wind[] = "Wind";
const char earth[] = "Earth";
const char fire[] = "Fire";
const char lightning[] = "Lightning";
const char plant[] = "Plant";
const char elder[] = "Elder";

const char *const typeNames[8] = {spirit, water, wind, earth, fire, lightning, plant, elder};

enum class Type {
    SPIRIT,
    WATER,
    WIND,
    EARTH,
    FIRE,
    LIGHTNING,
    PLANT,
    ELDER,
    NONE,
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
    constexpr DualType() : value(packTypes(Type::NONE, Type::NONE)) {}
    constexpr DualType(Type type) : value(packTypes(type, Type::NONE)) {}
    constexpr DualType(Type type1, Type type2) : value(packTypes(type1, type2)) {}

    constexpr Type getType1(void) const { return static_cast<Type>((this->value >> Type1Shift)); }

    constexpr Type getType2(void) const { return static_cast<Type>(this->value & Type2Mask); }
};

enum class Modifier : uint8_t {
    None,
    //  Quarter,
    Half,
    Same,
    Double,
    //	Quadruple,
};

static uint16_t applyMod(uint16_t value, Modifier modifier) {
    switch (modifier) {
    case Modifier::Same:
        return value;
    case Modifier::Half:
        return value >> 1;
    case Modifier::Double:
        return value << 1;
        //	case Modifier::Quarter: return value >> 2;
        //	case Modifier::Quadruple: return value << 2;
    default:
        return value;
    }
}

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
    return (attackType == Type::NONE || defendingType == Type::NONE)
               ? Modifier::None
               : static_cast<Modifier>(typeTable[static_cast<uint8_t>(attackType)][static_cast<uint8_t>(defendingType)]);
}

static uint16_t applyModifier(uint16_t baseValue, Type attackType, DualType defendingType) {
    const Modifier mod1 = getModifier(attackType, defendingType.getType1());
    baseValue = applyMod(baseValue, mod1);
    const Modifier mod2 = getModifier(attackType, defendingType.getType2());
    baseValue = applyMod(baseValue, mod2);
    return baseValue;
}