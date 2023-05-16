#include "World.hpp"
#include "../../sprites/characterSheet.h"
#include "../../sprites/tilesheet.h"
#include "../game/Gamestate.hpp"
#include "Arduboy2.h"
#include "Map.hpp"

#define TILE_SIZE 16

WorldEngine::WorldEngine(Arduboy2 *arduboy, GameState *state) {
  this->arduboy = arduboy;
  this->state = state;
  this->ticker = 1;
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
        Sprites::drawOverwrite(x * TILE_SIZE + this->mapx % TILE_SIZE - 9,
                               y * TILE_SIZE + this->mapy % TILE_SIZE - 8,
                               tilesheet, gameMap[tiley][tilex]);
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
  Sprites::drawOverwrite(PLAYER_X_OFFSET, PLAYER_Y_OFFSET, characterSheet,
                         frame);
}

void WorldEngine::runMap() {
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
  this->ticker++;
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

void WorldEngine::encounter() {
  TileType t = gameMap[this->cury][this->curx];
  if (t == TileType::GRASS) {
    int chance = random(1, 101);
    if (chance <= 50) {
      *this->state = GameState::FIGHT;
      this->ticker++;
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