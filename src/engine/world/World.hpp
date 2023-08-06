#pragma once
#include <stdint.h>

#include "../battle/Battle.hpp"
#include "../game/Gamestate.hpp"
#include "Encounter.hpp"
#include "Map.hpp"
#include <ArduboyFX.h>

// Notes: character center on map at all times
enum Direction { Up, Right, Down, Left };

class WorldEngine {
private:
  Arduboy2 *arduboy;
  Direction playerDirection;
  Encounter encounterTable;
  BattleEngine *battleEngine;
  int mapx, mapy;

  bool moving;
  uint8_t stepTicker;
  GameState_t *state;
  TileType nextTile;
  int curx, cury;
  int debug;

public:
  WorldEngine();
  WorldEngine(Arduboy2 *arduboy, GameState_t *state,
              BattleEngine *battleEngine);
  void input();
  void runMap();
  void drawMap();
  void drawPlayer();
  void moveChar();
  TileType getTile();
  void encounter();
  bool moveable();
};
