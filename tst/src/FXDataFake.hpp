#pragma once

#include "../../src/lib/uint24.h"
#include <stddef.h>
#include <stdint.h>

// Let the generated AVR header publish its real symbols in host tests.
using __uint24 = uint32_t;

namespace fxDataFake {
inline uint32_t readCount = 0;
inline uint24_t dataBase = 0;
inline uint24_t lastDataAddress = 0;
inline size_t lastDataLength = 0;
inline uint8_t dataBytes[16] = {};
}

namespace FX {
// Return the requested table byte address so callers' table selection and
// on-cart three-byte stride can be asserted independently of pixel data.
inline uint24_t readIndexedUInt24(uint24_t address, uint8_t index)
{
    ++fxDataFake::readCount;
    return address + static_cast<uint24_t>(index) * 3;
}

inline void readDataBytes(uint24_t address, uint8_t *buffer, size_t length)
{
    ++fxDataFake::readCount;
    fxDataFake::lastDataAddress = address;
    fxDataFake::lastDataLength = length;
    const uint32_t offset = address >= fxDataFake::dataBase
        ? address - fxDataFake::dataBase
        : UINT32_MAX;
    for (size_t i = 0; i < length; ++i) {
        const uint32_t byteOffset = offset + i;
        buffer[i] = byteOffset < sizeof(fxDataFake::dataBytes)
            ? fxDataFake::dataBytes[byteOffset]
            : 0;
    }
}
} // namespace FX
