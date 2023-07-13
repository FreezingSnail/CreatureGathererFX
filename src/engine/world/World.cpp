#include "World.hpp"

#include "../../fxdata/fxdata.h"
//#include "../../sprites/characterSheet.h"
#include "../battle/Battle.hpp"
#include "../game/Gamestate.hpp"
#include "Encounter.hpp"
#include "Map.hpp"
#include <ArduboyFX.h>

#define TILE_SIZE 8

WorldEngine::WorldEngine(Arduboy2 *arduboy, GameState_t *state,
                         BattleEngine *battleEngine) {
  this->arduboy = arduboy;
  this->encounterTable = Encounter(arduboy);
  this->battleEngine = battleEngine;
  this->state = state;
  this->mapx = 0;
  this->mapy = 0;
  this->curx = 4;
  this->cury = 2;
}

void WorldEngine::drawMap() {
  const int tileswide = (128 / TILE_SIZE) + 4;
  const int tilestall = (64 / TILE_SIZE) + 4;

  for (int y = 0; y < tilestall; y++) {
    for (int x = 0; x < tileswide; x++) {
      const int tilex = x - this->mapx / TILE_SIZE;
      const int tiley = y - this->mapy / TILE_SIZE;
      if (tilex >= 0 && tiley >= 0 && tilex < WORLD_WIDTH &&
          tiley < WORLD_HEIGHT) {
        FX::drawBitmap(x * TILE_SIZE + this->mapx % TILE_SIZE - 9,
                       y * TILE_SIZE + this->mapy % TILE_SIZE - 8, tilesheet,
                       gameMap[tiley][tilex], dbmNormal);
      }
    }
  }
}

void WorldEngine::input() {
  if (this->arduboy->pressed(UP_BUTTON)) {
    this->playerDirection = Up;
    if (this->moveable()) {
      this->moving = true;
    }
  } else if (this->arduboy->pressed(DOWN_BUTTON)) {
    this->playerDirection = Down;
    if (this->moveable()) {
      this->moving = true;
    }
  } else if (this->arduboy->pressed(LEFT_BUTTON)) {
    this->playerDirection = Left;
    if (this->moveable()) {
      this->moving = true;
    }
  } else if (this->arduboy->pressed(RIGHT_BUTTON)) {
    this->playerDirection = Right;
    if (this->moveable()) {
      this->moving = true;
    }
  }
}

#define PLAYER_SIZE 16
#define PLAYER_X_OFFSET WIDTH / 2 - PLAYER_SIZE / 2
#define PLAYER_Y_OFFSET HEIGHT / 2 - PLAYER_SIZE / 2
void WorldEngine::drawPlayer() {
  uint8_t frame =
      ((int)(this->playerDirection) * 3) + ((this->stepTicker - 1) / 5);
  FX::drawBitmap(PLAYER_X_OFFSET, PLAYER_Y_OFFSET, characterSheet, frame,
                 dbmNormal);
}

void __attribute__((optimize("-O0"))) WorldEngine::runMap() {
  this->drawMap();
  this->drawPlayer();

  if (this->moving) {
    this->moveChar();
  } else {
    this->input();
  }
}

void WorldEngine::moveChar() {
  switch (this->playerDirection) {
  case Up:
    this->mapy++;
    break;
  case Down:
    this->mapy--;
    break;
  case Left:
    this->mapx++;
    break;
  case Right:
    this->mapx--;
    break;
  }
  this->stepTicker++;
  if (this->stepTicker == TILE_SIZE) {
    this->stepTicker = 0;
    this->moving = false;
    switch (this->playerDirection) {
    case Up:
      this->cury--;
      break;
    case Down:
      this->cury++;
      break;
    case Left:
      this->curx--;
      break;
    case Right:
      this->curx++;
      break;
    }
    this->encounter();
  }
}

TileType WorldEngine::getTile() { return (TileType)0; }

void __attribute__((optimize("-O0"))) WorldEngine::encounter() {
  TileType t = gameMap[this->cury][this->curx];
  if (t == TileType::GRASS) {
    int chance = random(1, 101);
    if (chance <= 50) {
      uint8_t creatureID = this->encounterTable.rollEncounter();
      uint8_t level = this->encounterTable.rollLevel();
      this->debug = creatureID;
      this->battleEngine->startEncounter(creatureID, level);
      *this->state = GameState_t::BATTLE;
    }
  }
}

bool WorldEngine::moveable() {
  int tilex = this->curx;
  int tiley = this->cury;
  switch (this->playerDirection) {
  case Up:
    tiley--;
    break;
  case Down:
    tiley++;
    break;
  case Left:
    tilex--;
    break;
  case Right:
    tilex++;
    break;
  }

  if (tilex < 0 || tiley < 0 || tilex >= WORLD_WIDTH || tiley >= WORLD_HEIGHT) {
    return false;
  }
  this->nextTile = gameMap[tiley][tilex];
  switch (this->nextTile) {
  case WATER:
  case STONE:
    return false;

  default:
    return true;
  }
}
