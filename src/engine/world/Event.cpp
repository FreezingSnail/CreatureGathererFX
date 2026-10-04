#include "Event.hpp"
#include "../../lib/FxRead.hpp"
#include <stdint.h>

#include "../../fxdata.h"

void Event::loadEvent(uint8_t mapIndex, uint8_t subIndex, uint8_t eventIndex) {
    EventCords event;
    // map table adress
    uint24_t address = FxRead::indexed24(EventData::eventTable, mapIndex);

    // current submap  event adress
    address = FxRead::indexed24(address, subIndex) + sizeof(uint24_t) * 2 * eventIndex;

    this->textAddress = FxRead::indexed24(address, 0);
    FxRead::object(FxRead::indexed24(address, 1), event);
    this->cords = event;
    FxReadCounter::transitionExact(5);
}
