#pragma once
#include "Move.hpp"
#include "DataTypes.hpp"
#include "../creature/Creature.hpp"
#include "uint24.h"

Move readMoveFX(uint8_t index);

uint24_t readCreatureNameAddress(uint8_t id);
uint24_t readMoveNameAddress(uint16_t id);
uint24_t readEffectStringAddress();
uint8_t readCreatureNameWidth(uint8_t id);
uint8_t readMoveNameWidth(uint16_t id);
uint8_t readEffectStringWidth();

// TODO: The rates dont exist yet in flash data
uint8_t getEffectRateFX(uint8_t id);

// TODO: add a bitarray for the effect targets
bool selfEffect(Effect effect);

OpponentSeed readOpponentSeed(uint8_t index);

CreatureData_t getCreatureFromStore(uint8_t id);

void arenaLoad(Creature *creature, uint24_t addr, uint8_t lvl);

uint16_t ReadFXu16(uint24_t addr);
