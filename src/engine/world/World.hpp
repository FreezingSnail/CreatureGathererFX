#pragma once

#include <stdint.h>

#include "../ModeState.hpp"
#include "Chunk.hpp"

#define EVENTCOUNT 6

enum class Direction : uint8_t {
    UP,
    RIGHT,
    DOWN,
    LEFT
};

struct ViewOffset {
    int8_t x;
    int8_t y;
    uint8_t mask;
};

// Stateless world operations use the active WorldTransient storage. Persistent
// coordinates live in GameState, outside ModeState.
class WorldEngine {
  public:
    static void init(WorldTransient &world);
    static void input(WorldTransient &world);
    static void runMap(WorldTransient &world);
    static void moveChar(WorldTransient &world);
    static uint8_t getTile();
    static void encounter();
    static bool moveable(const WorldTransient &world);
    static void interact();

    static void loadMap(WorldTransient &world, uint8_t mapIndex, uint8_t submapIndex);
    static void setPos(WorldTransient &world, uint8_t x, uint8_t y);
    static ViewOffset view(const WorldTransient &world);
    static uint16_t location();
    static void syncFromLocation(WorldTransient &world);
#ifdef TEST
    static void beginMoveForTest(WorldTransient &world, Direction direction);
#endif

  private:
    static void onChunkChange(WorldTransient &world, uint16_t newChunk);
};
