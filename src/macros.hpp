#pragma once

#ifdef DEBUG
#define printd(...) printf(__VA_ARGS__)
#else
#define printd(...)
#endif

#define WALKINGFLAG 0b00000001
#define BIT2 0b00000010
#define BIT3 0b00000100
#define BIT4 0b00001000
#define BIT5 0b00010000
#define BIT6 0b00100000
#define BIT7 0b01000000
#define BIT8 0b10000000

#define SET_FLAG(position, flags) (flags |= (1 << position))
#define CLEAR_FLAG(position, flags) (flags &= ~(1 << position))
#define TOGGLE_FLAG(position, flags) (flags ^= (1 << position))
#define IS_SET_FLAG(position, flags) ((flags & (1 << position)) != 0)

