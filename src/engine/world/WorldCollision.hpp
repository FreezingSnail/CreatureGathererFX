#pragma once

#include <stdint.h>

#include "TilePropertyWindow.hpp"

namespace WorldCollision {

inline bool canEnter(int16_t x, int16_t y, uint16_t mapWidth, uint16_t mapHeight,
                     const uint8_t *windowStorage) {
    if (x < 0 || y < 0 || x >= static_cast<int16_t>(mapWidth) ||
        y >= static_cast<int16_t>(mapHeight)) {
        return false;
    }

    uint8_t properties = 0;
    bool occupied = false;
    const TilePropertyWindow window(windowStorage);
    return window.lookup(x, y, properties, occupied) && occupied &&
           (properties & TileProps::PROP_WALKABLE) != 0;
}

} // namespace WorldCollision
