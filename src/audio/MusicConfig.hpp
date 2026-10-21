#pragma once

// Compile-time music runtime configuration. The Makefile passes AUDIO=1 as
// CGFX_AUDIO=1 in both C and C++ flags; every other build (and every host TEST
// build) keeps the runtime off. Do not add runtime state here.
#ifndef CGFX_AUDIO
#define CGFX_AUDIO 0
#endif
