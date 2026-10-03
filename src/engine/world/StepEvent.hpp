#pragma once

#include <stdint.h>

#include "Encounter.hpp"

void onStep(uint16_t tile);

#ifdef TEST
namespace StepEventTest {
void setEncounterRng(Encounter::Rng rng);
}
#endif
