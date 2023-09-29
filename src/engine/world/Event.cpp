#include "Event.hpp"
#include <ArduboyFX.h>
#include <stdint.h>

#include "../../fxdata/fxdata.h"

void __attribute__((optimize("-O0"))) Event::loadEvent(uint8_t mapIndex, uint8_t eventIndex) {
    uint24_t address = EventData::eventTable + sizeof(uint24_t) * mapIndex;
    address = FX::readIndexedUInt24(address, eventIndex);
    FX::readDataObject(FX::readIndexedUInt24(address,0), this->cords);
    this->textAddress = FX::readIndexedUInt24(address, 1);
}