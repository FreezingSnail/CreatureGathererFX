#include "ReadData.hpp"
#include <ArduboyFX.h>
#include "../fxdata.h"

MoveBitSet getMovePack(uint8_t index) {
    MoveBitSet move;
    uint24_t rowAddress = MoveData::movePack + sizeof(MoveBitSet) * index;
    FX::readDataObject(rowAddress, move);
    return move;
}

// TODO: The rates dont exist yet in flash data
uint8_t getEffectRate(Effect effect) {
    uint8_t rate;
    // uint24_t rowAddress = MoveData::effectRates + sizeof(uint8_t) * static_cast<uint8_t>(effect);
    // FX::readDataObject(rowAddress, rate);
    // return rate;
    return 100;
}

// TODO: add a bitarray for the effect targets
bool selfEffect(Effect effect) {
    uint32_t effectTargets;
    // FX::readDataObject(MoveData::selfEffect, effectTargets);
    return effectTargets >> uint8_t(effect) & 1 == 1;
}

inline OpponentSeed_t reeadOpponentSeed(uint8_t index) {
    OpponentSeed_t seed = OpponentSeed_t{0, 0, 1};
    uint24_t rowAddress = FX::readIndexedUInt24(opts, index);
    FX::readDataObject(rowAddress, seed);
    return seed;
}