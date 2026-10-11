#include "Type.hpp"

namespace {
// 81 cells at two bits each: 0=None, 1=Half, 2=Same, 3=Double.
const uint8_t packedTypeTable[21] TYPE_TABLE_STORAGE = {
    0xaa, 0x2a, 0xe8, 0x6d, 0xa1, 0x5e, 0x87,
    0xa7, 0x17, 0x72, 0xbe, 0xf8, 0xf9, 0x22,
    0xe7, 0x8b, 0x9c, 0x2f, 0x00, 0x00, 0x00,
};
}

Modifier getModifier(Type attackType, Type defendingType) {
    if (defendingType == Type::NONE) return Modifier::Same;
    if (attackType == Type::NONE) return Modifier::None;
    const uint8_t index = static_cast<uint8_t>(attackType) * TypeCount +
                          static_cast<uint8_t>(defendingType);
#ifdef __AVR__
    const uint8_t packed = pgm_read_byte(packedTypeTable + (index >> 2));
#else
    const uint8_t packed = packedTypeTable[index >> 2];
#endif
    const uint8_t cell = (packed >> ((index & 3u) << 1)) & 3u;
    return static_cast<Modifier>(cell + (cell != 0));
}
