#pragma once

#include <stdint.h>

#ifndef PROGMEM
#define PROGMEM
#endif

inline uint8_t pgm_read_byte(const void *address)
{
    return *static_cast<const uint8_t *>(address);
}

inline uint16_t pgm_read_word(const void *address)
{
    return *static_cast<const uint16_t *>(address);
}
