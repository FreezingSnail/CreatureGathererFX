#pragma once
#include "../lib/Move.hpp"
#include "Arduboy2.h"

static void printMoveInfo(Arduboy2 *arduboy, uint8_t index) {
  MoveBitSet m = getMovePack(index);
  arduboy->setCursor(38, 4);

  arduboy->print(readFlashStringPointer(&typeNames[m.type]));
  arduboy->setCursor(38, 12);
  if (m.physical) {
    arduboy->print(F("Phys"));
  } else {
    arduboy->print(F("Spec"));
  }
  arduboy->setCursor(38, 19);
  arduboy->print(F("power:"));
  arduboy->print(m.power);
}