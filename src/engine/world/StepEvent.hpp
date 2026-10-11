#pragma once

#include <stdint.h>

#include "Encounter.hpp"

void onStep(uint16_t tile);

#if defined(TEST) || defined(FX_READ_COUNTER)
namespace StepEventTest {
void setEncounterRng(Encounter::Rng rng);
}
#endif
