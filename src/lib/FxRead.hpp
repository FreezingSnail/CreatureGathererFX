#pragma once

#include "FxReadCounter.hpp"

#ifdef TEST
#include "../../tst/src/FXDataFake.hpp"
#else
#include <ArduboyFX.h>
#endif

// All ordinary program-data lookups pass through this boundary. Each wrapper
// records one operation, regardless of its byte count or SPI seek phases.
namespace FxRead {

inline uint24_t indexed24(uint24_t address, uint16_t index) {
    FxReadCounter::record();
#ifdef TEST
    return FX::readIndexedUInt24(address, index);
#else
    // ArduboyFX 1.4.0's AVR readIndexedUInt24/readPendingLastUInt24 path
    // does not preserve the high table byte. Decode the packed big-endian
    // entry from raw bytes instead, keeping the cart's full 24-bit address.
    uint8_t bytes[3];
    FX::readDataBytes(address + static_cast<uint24_t>(index) * 3, bytes,
                      sizeof(bytes));
    return (static_cast<uint24_t>(bytes[0]) << 16) |
           (static_cast<uint24_t>(bytes[1]) << 8) | bytes[2];
#endif
}

inline void bytes(uint24_t address, uint8_t *destination, size_t length) {
    FxReadCounter::record();
    FX::readDataBytes(address, destination, length);
}

#ifndef TEST
inline uint8_t indexed8(uint24_t address, uint16_t index) {
    FxReadCounter::record();
    return FX::readIndexedUInt8(address, index);
}

inline uint16_t indexed16(uint24_t address, uint16_t index) {
    FxReadCounter::record();
    return FX::readIndexedUInt16(address, index);
}

inline uint32_t indexed32(uint24_t address, uint16_t index) {
    FxReadCounter::record();
    return FX::readIndexedUInt32(address, index);
}

template <typename T> inline void object(uint24_t address, T &destination) {
    FxReadCounter::record();
    FX::readDataObject(address, destination);
}

inline uint16_t littleEndian16(uint24_t address) {
    FxReadCounter::record();
    FX::seekData(address);
    uint8_t data[2];
    FX::readBytes(data, sizeof(data));
    FX::readEnd();
    return static_cast<uint16_t>(data[1]) << 8 | data[0];
}

struct SpriteHeader { uint8_t width; uint8_t height; };
inline SpriteHeader spriteHeader(uint24_t address) {
    FxReadCounter::record();
    FX::seekData(address);
    const uint8_t width = FX::readPendingUInt8();
    return {width, FX::readEnd()};
}
#endif

} // namespace FxRead
