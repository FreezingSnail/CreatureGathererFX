#pragma once
#include "../game/Gamestate.hpp"
#include "Arduboy2.h"
#include <stdint.h>

// Notes: character center on map at all times
enum Direction { Up, Down, Left, Right };

class WorldEngine {
private:
  Arduboy2 *arduboy;
  Direction playerDirection;
  int mapx, mapy;
  // ticker for encounter rate
  uint8_t ticker;
  GameState *state;

public:
  WorldEngine(Arduboy2 *arduboy, GameState *state);
  void input();
  void runMap();
  void drawMap();
  void moveChar(int x, int y);
  void encounter();
};