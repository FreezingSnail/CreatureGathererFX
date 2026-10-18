#pragma once
#include "Effect.hpp"
#include "DataTypes.hpp"
#include "../creature/Creature.hpp"
#include "uint24.h"

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

// The authored per-species duel kit mask. Lives here so both the battle and
// compaction evolution paths adapt one shared read of the target species.
uint8_t readCreatureMask(uint8_t id);

uint16_t ReadFXu16(uint24_t addr);
