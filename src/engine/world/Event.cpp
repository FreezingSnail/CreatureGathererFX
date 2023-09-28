#include "Event.hpp"
#include <ArduboyFX.h>
#include <stdint.h>

#include "../../fxdata/fxdata.h"

void __attribute__((optimize("-O0"))) Event::loadEvent(uint8_t index) {
    EventPack event;
    uint24_t address = EventData::eventLocationTable;
    FX::readDataObject(address, event);
    this->event = event;
    address = FX::readIndexedUInt24(EventData::eventTextTable, index);
    this->event.textAddress = address;
    FX::readDataBytes(address, this->text, 20);
}