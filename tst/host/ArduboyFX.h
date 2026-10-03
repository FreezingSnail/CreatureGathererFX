#pragma once

#include <stddef.h>
#include <stdint.h>

namespace FX {
uint32_t readIndexedUInt24(uint32_t address, uint8_t index);
void readDataBytes(uint32_t address, uint8_t *buffer, size_t length);
}
