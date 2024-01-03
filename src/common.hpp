#pragma once

#define SPRITESU_RECT
#include "external/SpritesU.hpp"

#include <Arduboy2.h>
extern Arduboy2Base arduboy;

#include "external/Font4x6.h"
extern Font4x6 font;
#include "lib/Text.hpp"

#define DGF __attribute__((optimize("-O0")))
