#pragma once
#include "../game/Gamestate.hpp"
#include "Arduboy2.h"
#include "Map.hpp"
#include <stdint.h>

// Notes: character center on map at all times
enum Direction { Up, Right, Down, Left };

class WorldEngine {
private:
  Arduboy2 *arduboy;
  Direction playerDirection;
  int mapx, mapy;
  // ticker for encounter rate
  uint8_t ticker;
  bool moving;
  uint8_t stepTicker;
  GameState *state;
  TileType nextTile;
  int curx, cury;

public:
  WorldEngine(Arduboy2 *arduboy, GameState *state);
  void input();
  void runMap();
  void drawMap();
  void drawPlayer();
  void moveChar();
  TileType getTile();
  void encounter();
  bool moveable();
};