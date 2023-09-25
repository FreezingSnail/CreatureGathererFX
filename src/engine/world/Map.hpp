#pragma once

#define WORLD_WIDTH 20
#define WORLD_HEIGHT 30
enum TileType { GRASS=0, WATER=8, TREES=16, STONE=24 };

static uint8_t gameMap[30][20] = {};
