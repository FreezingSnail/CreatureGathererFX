#pragma once

#include "../../lib/ReadData.hpp"

namespace battle {

inline void cacheCreatureName(uint24_t &address, uint8_t &width,
                              uint8_t species) {
    const uint8_t sourceWidth = readCreatureNameWidth(species);
    const uint24_t source = sourceWidth == 0
        ? 0 : readCreatureNameAddress(species);
    address = source == 0 || sourceWidth == 0 ? 0 : source;
    width = source == 0 || sourceWidth == 0 ? 0 : sourceWidth;
}

} // namespace battle
