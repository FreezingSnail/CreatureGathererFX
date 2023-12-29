#pragma once

#define ABG_TIMER1
#define ABG_SYNC_PARK_ROW

#include "external/ArduboyG.h"
extern ArduboyGBase_Config<ABG_Mode::L3> arduboy;

#define SPRITESU_RECT
#define SPRITESU_FX
#define SPRITESU_RECT
#include "external/SpritesU.hpp"

#include "external/Font4x6.h"
extern Font4x6 font;
#include "lib/Text.hpp"

#define FRAMEGRAY(f) ((f * 3) + a.currentPlane())
#define FRAME(f) ((f * 3))
