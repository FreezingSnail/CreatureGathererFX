#pragma once

#define WORLD_WIDTH 20
#define WORLD_HEIGHT 30
enum TileType { GRASS = 0, WATER = 8, TREES = 16, STONE = 24 };

// TODO extreme mem waste in the worst of ways
// screen only fits 8x4 tiles
// one option is to bit pack the map
// relative loading also would help
// 4x4 chuncks, -> 12 chunks at anytime should be held
// without bit backing thats 192 tiles or 192 bytes vs the current 600
static uint8_t gameMap[0][0] = {};
