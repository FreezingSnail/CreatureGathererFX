#pragma once

#define ABG_TIMER1
#define ABG_SYNC_PARK_ROW

#include "external/ArduboyG.h"
extern ArduboyGBase_Config<ABG_Mode::L4_Triplane> arduboy;

#define SPRITESU_OVERWRITE
#define SPRITESU_PLUSMASK
#define SPRITESU_RECT
#define SPRITESU_FX
#include "external/SpritesU.hpp"

#include "external/Font4x6.h"
extern Font4x6 font;
#include "lib/Text.hpp"

#include "Animator.hpp"
extern Animator animator;


#define FRAME(x) x * 3 + arduboy.currentPlane()

#define DGF __attribute__((optimize("-O0")))
