#pragma once
#include <ArduboyFX.h>
#include <stdint.h>

struct EventPack {
    uint8_t x, y, spriteIndex;
    uint24_t textAddress;
};

class Event {
  public:
    EventPack event;
    uint8_t text[30];

    void loadEvent(uint8_t index);
};
