#include "Event.hpp"
#include <ArduboyFX.h>
#include <stdint.h>

#include "../../fxdata/fxdata.h"

void __attribute__((optimize("-O0"))) Event::loadEvent(uint8_t mapIndex, uint8_t subIndex, uint8_t eventIndex) {
    EventCords event;
    // map table adress
    uint24_t address = FX::readIndexedUInt24(EventData::eventTable, mapIndex);

    // current submap  event adress
    address = FX::readIndexedUInt24(address, subIndex) + sizeof(uint24_t) * 2 * eventIndex;

    this->textAddress = FX::readIndexedUInt24(address, 0);
    FX::readDataObject(FX::readIndexedUInt24(address, 1), event);
    this->cords = event;
}