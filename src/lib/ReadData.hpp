#pragma once
#include "Move.hpp"
#include "DataTypes.hpp"

MoveBitSet getMovePack(uint8_t index);

// TODO: The rates dont exist yet in flash data
uint8_t getEffectRate(Effect effect);

// TODO: add a bitarray for the effect targets
bool selfEffect(Effect effect);

OpponentSeed_t reeadOpponentSeed(uint8_t index);