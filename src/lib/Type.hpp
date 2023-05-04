typedef enum class Types{
	spirit,
	water,
	wind,
	earth,
	fire,
	lightning,
	plant,
	elder,
	none,
}Type_t;

constexpr const uint8_t TypeCount = 9;

class DualType
{
private:
	uint8_t value;

	constexpr const uint8_t Type1Mask = 0b11100000;
	constexpr const uint8_t Type2Mask = 0b00001111;
	constexpr const uint8_t Type1Shift = 4;
	constexpr const uint8_t Type2Shift = 0;

	constexpr uint8_t packTypes(Type type1, Type type2)
	{
		return ((static_cast<uint8_t>(type1) & Type1Mask) << Type1Shift) | ((static_cast<uint8_t>(type2) & Type2Mask) << Type2Shift);
	}
  
public:
	constexpr DualType(Type type) : value(packTypes(type, Type::None)) {}
	constexpr DualType(Type type1, Type type2) : value(packTypes(type1, type2)) {}
	
	constexpr Type getType1(void) const
	{
		return static_cast<Type>((this->value >> Type1Shift) & Type1Mask);
	}
	
	constexpr Type getType2(void) const
	{
		return static_cast<Type>((this->value >> Type2Shift) & Type2Mask);
	}
};

enum class Modifier : uint8_t
{
    None,
    Quarter,
	Half,
    Same,
	Double,
	Quadruple,
};

uint16_t applyModifier(uint16_t value, Modifier modifier)
{
	switch(modifier)
	{
		case Modifier::Same: return value;
		case Modifier::Half: return value >> 1;
		case Modifier::Double: return value << 1;
		case Modifier::Quarter: return value >> 2;
		case Modifier::Quadruple: return value << 2;
		default: return value;
	}
}

Modifier typeTable[TypeCount][TypeCount] PROGMEM =
{
	// // Fire
	// {
	// 	Modifier::Half, // Fire
	// 	Modifier::Quarter, // Water
	// 	Modifier::Same, // Electric
	// 	Modifier::Double, // Wind
	// },
	// // Water
	// {
	// 	Modifier::Double, // Fire
	// 	Modifier::Same, // Water
	// 	Modifier::Same, // Electric
	// 	Modifier::Half, // Wind
	// },
	// // Electric
	// {
	// 	Modifier::Same, // Fire
	// 	Modifier::Quadruple, // Water
	// 	Modifier::Same, // Electric
	// 	Modifier::Half, // Wind
	// },
	// // Wind
	// {
	// 	Modifier::Half, // Fire
	// 	Modifier::Double, // Water
	// 	Modifier::Same, // Electric
	// 	Modifier::Half, // Wind
	// },
};


Modifier getModifier(Type attackType, Type defendingType)
{
	return (attackType == Type::None || defendingType == Type::None)
	? Modifier::None :
	static_cast<Modifier>(pgm_read_byte(&typeTable[static_cast<uint8_t>(attackType)][static_cast<uint8_t>(defendingType)]));
}

uint16_t handleAttack(uint16_t baseValue, Type attackType, DualType defendingType)
{
	const Modifier mod1 = getModifier(attackType, defendingType.getType1());
	baseValue = applyModifier(baseValue, mod1);
	const Modifier mod2 = getModifier(attackType, defendingType.getType2());
	baseValue = applyModifier(baseValue, mod2);
	return baseValue;
}