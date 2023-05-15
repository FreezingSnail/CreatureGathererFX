
#include "World.hpp"
#include "Arduboy2.h"
#include "../../sprites/tilesheet.h"
#include "Map.hpp"

#define TILE_SIZE 16

WorldEngine::WorldEngine(Arduboy2* arduboy){
  this->arduboy = arduboy;
}

void WorldEngine::drawMap() {
  const int tileswide = 128 / TILE_SIZE + 2;
  const int tilestall = 64 / TILE_SIZE + 2;

  for (int y = 0; y < tilestall; y++) {
    for (int x = 0; x < tileswide; x++) {
      const int tilex = x - mapx / TILE_SIZE;
      const int tiley = y - mapy / TILE_SIZE;
      if (tilex >= 0 && tiley >= 0 && tilex < WORLD_WIDTH && tiley < WORLD_HEIGHT) {
        Sprites::drawOverwrite(x * TILE_SIZE + this->mapx % TILE_SIZE, y * TILE_SIZE + this->mapy % TILE_SIZE, tilesheet, gameMap[tiley][tilex]);
      }
    }
  }
}

void WorldEngine::input() {
  if (this->arduboy->pressed(UP_BUTTON)) {
    this->mapy += 1;
  }
  if (this->arduboy->pressed(DOWN_BUTTON)) {
    this->mapy -= 1;
  }
  if (this->arduboy->pressed(LEFT_BUTTON)) {
    this->mapx += 1;
  }
  if (this->arduboy->pressed(RIGHT_BUTTON)) {
    this->mapx -= 1;
  }
}

#define PLAYER_SIZE      16
#define PLAYER_X_OFFSET    WIDTH / 2 - PLAYER_SIZE / 2
#define PLAYER_Y_OFFSET    HEIGHT / 2 - PLAYER_SIZE / 2
void drawplayer(Arduboy2* arduboy) {
  arduboy->fillRect(PLAYER_X_OFFSET, PLAYER_Y_OFFSET, PLAYER_SIZE, PLAYER_SIZE, BLACK);
}

void WorldEngine::runMap() {

  this->input();
  this->drawMap();
  drawplayer(this->arduboy);

}