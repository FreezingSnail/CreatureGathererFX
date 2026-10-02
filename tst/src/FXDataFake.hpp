#pragma once

#include "../../src/lib/uint24.h"

// Let the generated AVR header publish its real symbols in host tests.
using __uint24 = uint32_t;

namespace fxDataFake {
inline uint32_t readCount = 0;
}

namespace FX {
// Return the requested table byte address so callers' table selection and
// on-cart three-byte stride can be asserted independently of pixel data.
inline uint24_t readIndexedUInt24(uint24_t address, uint8_t index)
{
    ++fxDataFake::readCount;
    return address + static_cast<uint24_t>(index) * 3;
}
} // namespace FX
