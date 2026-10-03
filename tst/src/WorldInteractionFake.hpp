#pragma once

#include "../../src/lib/uint24.h"
#include <stdint.h>

namespace worldInteractionFake {
extern bool pressedA;
extern uint24_t scriptsBase;
extern uint8_t slotBytes[128];
extern uint16_t readCount;
extern uint24_t lastAddress;
extern uint8_t lastLength;
extern uint8_t *lastScript;
extern uint16_t runCount;
extern uint16_t currentTile;
extern uint16_t targetTile;
extern uint8_t runFirstByte;

void reset();
}
