#pragma once
#include <ArduboyFX.h>
#include "Move.hpp"
#include "../fxdata.h"

static MoveBitSet getMovePack(uint8_t index) {
    MoveBitSet move;
    uint24_t rowAddress = MoveData::movePack + sizeof(MoveBitSet) * index;
    FX::readDataObject(rowAddress, move);
    return move;
}
