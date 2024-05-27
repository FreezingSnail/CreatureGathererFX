#pragma once
#include "Move.hpp"
#include "DataTypes.hpp"
#include "../creature/Creature.hpp"
#include "../opponent/Opponent.hpp"

using uint24_t = __uint24;

MoveBitSet getMovePack(uint8_t index);

// TODO: The rates dont exist yet in flash data
uint8_t getEffectRate(Effect effect);

// TODO: add a bitarray for the effect targets
bool selfEffect(Effect effect);

OpponentSeed_t reeadOpponentSeed(uint8_t index);

CreatureData_t getCreatureFromStore(uint8_t id);

void arenaLoad(Creature *creature, uint24_t addr, uint8_t lvl);

void ReadOpt(Opponent *opt, uint8_t index);
void loadEncounterOpt(Opponent *opt, uint8_t creatureID, uint8_t level);
