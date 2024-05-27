#pragma once
#include <stdint.h>
#ifdef __AVR__
typedef __uint24 uint24_t;
#else
typedef uint32_t uint24_t;
#endif