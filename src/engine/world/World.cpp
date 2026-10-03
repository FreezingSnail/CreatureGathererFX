#include "World.hpp"
#include "WorldCollision.hpp"
#include "StepEvent.hpp"

#ifndef TEST
#include "../../common.hpp"
#include "../../globals.hpp"
#include "../../lib/FxRead.hpp"
#else
#include "../../GameState.hpp"
extern GameState gameState;
extern bool worldInteractionJustPressedA();
extern bool worldInteractionDialogActive();
extern void worldInteractionPopDialog();
extern void worldInteractionReadScript(uint24_t address, uint8_t *buffer, uint8_t length);
extern void worldInteractionRunScript(uint8_t *script, uint16_t currentTile, uint16_t targetTile);
extern uint24_t worldInteractionScriptsBase();
extern bool worldMovementDirection(Direction &value);
#endif

namespace {
constexpr uint8_t TILE_SIZE = 16;
constexpr uint8_t DIRECTION_MASK = 0x03;
constexpr uint8_t MOVING_MASK = 0x04;
constexpr uint8_t WALK_MASK_BITS = 0x38;
constexpr uint8_t WALK_SHIFT = 3;

enum WalkMaskIndex : uint8_t {
    WALK_NONE = 0,
    WALK_LEFT = 1,
    WALK_RIGHT = 2,
    WALK_UP = 3,
    WALK_DOWN = 4
};

Direction direction(const WorldMotion &motion) {
    return static_cast<Direction>(motion.directionAndFlags & DIRECTION_MASK);
}

bool moving(const WorldMotion &motion) {
    return (motion.directionAndFlags & MOVING_MASK) != 0;
}

void setDirection(WorldMotion &motion, Direction value) {
    motion.directionAndFlags = static_cast<uint8_t>(
        (motion.directionAndFlags & ~DIRECTION_MASK) | static_cast<uint8_t>(value));
}

void setMoving(WorldMotion &motion, bool value) {
    if (value) motion.directionAndFlags |= MOVING_MASK;
    else motion.directionAndFlags &= static_cast<uint8_t>(~MOVING_MASK);
}

void setWalkMask(WorldMotion &motion, WalkMaskIndex value) {
    motion.directionAndFlags = static_cast<uint8_t>(
        (motion.directionAndFlags & ~WALK_MASK_BITS) |
        (static_cast<uint8_t>(value) << WALK_SHIFT));
}

uint8_t walkMask(const WorldMotion &motion) {
    switch ((motion.directionAndFlags & WALK_MASK_BITS) >> WALK_SHIFT) {
    case WALK_LEFT: return 0b10000000;
    case WALK_RIGHT: return 0b01000000;
    case WALK_UP: return 0b00100000;
    case WALK_DOWN: return 0b00001000;
    default: return 0;
    }
}

WalkMaskIndex walkMaskFor(Direction value) {
    switch (value) {
    case Direction::LEFT: return WALK_LEFT;
    case Direction::RIGHT: return WALK_RIGHT;
    case Direction::UP: return WALK_UP;
    case Direction::DOWN: return WALK_DOWN;
    }
    return WALK_NONE;
}

uint16_t origin(const WorldMotion &motion) {
    return static_cast<uint16_t>(motion.originLow) |
           (static_cast<uint16_t>(motion.originHigh) << 8);
}

void setOrigin(WorldMotion &motion, uint16_t value) {
    motion.originLow = static_cast<uint8_t>(value & 0xFF);
    motion.originHigh = static_cast<uint8_t>(value >> 8);
}

void clearStep(WorldMotion &motion, uint16_t newOrigin) {
    motion.step = 0;
    setMoving(motion, false);
    setWalkMask(motion, WALK_NONE);
    setOrigin(motion, newOrigin);
}

bool interactionJustPressedA() {
#ifdef TEST
    return worldInteractionJustPressedA();
#else
    return arduboy.justPressed(A_BUTTON);
#endif
}

bool interactionDialogActive() {
#ifdef TEST
    return worldInteractionDialogActive();
#else
    return dialogMenu.peek();
#endif
}

void popInteractionDialog() {
#ifdef TEST
    worldInteractionPopDialog();
#else
    dialogMenu.popMenu();
#endif
}

void readInteractionScript(uint24_t address, uint8_t *script, uint8_t length) {
#ifdef TEST
    worldInteractionReadScript(address, script, length);
#else
    FxRead::bytes(address, script, length);
    FxReadCounter::transitionExact(1);
#endif
}

void runInteractionScript(uint8_t *script, uint16_t currentTile, uint16_t targetTile) {
#ifdef TEST
    worldInteractionRunScript(script, currentTile, targetTile);
#else
    vm.initVM(script, currentTile, targetTile);
    vm.run();
#endif
}

uint24_t interactionScriptsBase() {
#ifdef TEST
    return worldInteractionScriptsBase();
#else
    return scripts;
#endif
}

bool facedTile(uint16_t playerLocation, Direction facing, uint16_t &target) {
    int16_t x = static_cast<uint8_t>(playerLocation & 0xFF);
    int16_t y = static_cast<uint8_t>(playerLocation >> 8);
    switch (facing) {
    case Direction::UP: --y; break;
    case Direction::DOWN: ++y; break;
    case Direction::LEFT: --x; break;
    case Direction::RIGHT: ++x; break;
    }
    if (x < 0 || y < 0 || x >= Chunk::MAP_WIDTH_TILES || y >= Chunk::MAP_HEIGHT_TILES) {
        return false;
    }
    target = static_cast<uint16_t>((static_cast<uint16_t>(y) << 8) |
                                   static_cast<uint16_t>(x));
    return true;
}
}

void WorldEngine::init(WorldTransient &world) {
    world.activateMotion();
    TilePropertyWindow::invalidate(world.propertyWindow);
    world.motion.directionAndFlags = static_cast<uint8_t>(Direction::DOWN);
    world.motion.step = 0;
    setMoving(world.motion, false);
    setOrigin(world.motion, gameState.playerLocation);
}

void WorldEngine::loadMap(WorldTransient &world, uint8_t mapIndex, uint8_t submapIndex) {
    // The FX image contains one canonical map; it occupies map/submap 0/0.
    (void)mapIndex;
    (void)submapIndex;
    TilePropertyWindow::invalidate(world.propertyWindow);
}

void WorldEngine::setPos(WorldTransient &world, uint8_t x, uint8_t y) {
    gameState.playerLocation = static_cast<uint16_t>(x) |
                               (static_cast<uint16_t>(y) << 8);
    clearStep(world.motion, gameState.playerLocation);
    TilePropertyWindow::invalidate(world.propertyWindow);
}

uint16_t WorldEngine::location() {
    return gameState.playerLocation;
}

ViewOffset WorldEngine::view(const WorldTransient &world) {
    const WorldMotion &motion = world.motion;
    int8_t x = 0;
    int8_t y = 0;
    switch (direction(motion)) {
    case Direction::UP: y = static_cast<int8_t>(motion.step); break;
    case Direction::DOWN: y = -static_cast<int8_t>(motion.step); break;
    case Direction::LEFT: x = static_cast<int8_t>(motion.step); break;
    case Direction::RIGHT: x = -static_cast<int8_t>(motion.step); break;
    }
    ViewOffset result = {x, y, walkMask(motion)};
    return result;
}

void WorldEngine::syncFromLocation(WorldTransient &world) {
    const uint16_t published = gameState.playerLocation;
    if (published == origin(world.motion)) return;
    clearStep(world.motion, published);
    TilePropertyWindow::invalidate(world.propertyWindow);
}

void WorldEngine::input(WorldTransient &world) {
#ifdef TEST
    Direction requested;
    if (worldMovementDirection(requested)) {
        setDirection(world.motion, requested);
        if (moveable(world)) {
            setMoving(world.motion, true);
            setWalkMask(world.motion, walkMaskFor(requested));
        }
    } else {
        setMoving(world.motion, false);
    }
#else
    WorldMotion &motion = world.motion;
    if (arduboy.pressed(LEFT_BUTTON)) {
        setDirection(motion, Direction::LEFT);
        if (moveable(world)) {
            setMoving(motion, true);
            setWalkMask(motion, walkMaskFor(Direction::LEFT));
        }
    } else if (arduboy.pressed(RIGHT_BUTTON)) {
        setDirection(motion, Direction::RIGHT);
        if (moveable(world)) {
            setMoving(motion, true);
            setWalkMask(motion, walkMaskFor(Direction::RIGHT));
        }
    } else if (arduboy.pressed(UP_BUTTON)) {
        setDirection(motion, Direction::UP);
        if (moveable(world)) {
            setMoving(motion, true);
            setWalkMask(motion, walkMaskFor(Direction::UP));
        }
    } else if (arduboy.pressed(DOWN_BUTTON)) {
        setDirection(motion, Direction::DOWN);
        if (moveable(world)) {
            setMoving(motion, true);
            setWalkMask(motion, walkMaskFor(Direction::DOWN));
        }
    } else {
        setMoving(motion, false);
    }
#endif
}

void WorldEngine::runMap(WorldTransient &world) {
    syncFromLocation(world);
    if (interactionDialogActive()) {
        if (interactionJustPressedA()) popInteractionDialog();
    } else if (moving(world.motion) && moveable(world)) {
        moveChar(world);
    } else {
        const uint16_t beforeInteract = gameState.playerLocation;
        interact();
        if (gameState.playerLocation != beforeInteract) syncFromLocation(world);
        if (!interactionDialogActive()) input(world);
        // Match the previous sketch path: the first animation pixel is applied
        // on the same frame that accepts the direction, for 16 ticks per tile.
        if (!interactionDialogActive() && moving(world.motion) && moveable(world)) {
            moveChar(world);
        }
    }
}

void WorldEngine::moveChar(WorldTransient &world) {
    WorldMotion &motion = world.motion;
    if (!moving(motion)) return;
    const Direction moveDirection = direction(motion);
    ++motion.step;
    if (motion.step != TILE_SIZE) return;

    const uint16_t oldLocation = gameState.playerLocation;
    int16_t x = static_cast<uint8_t>(oldLocation & 0xFF);
    int16_t y = static_cast<uint8_t>(oldLocation >> 8);
    switch (moveDirection) {
    case Direction::UP: --y; break;
    case Direction::DOWN: ++y; break;
    case Direction::LEFT: --x; break;
    case Direction::RIGHT: ++x; break;
    }
    const uint16_t newLocation = static_cast<uint16_t>(x) |
                                 (static_cast<uint16_t>(y) << 8);
    gameState.playerLocation = newLocation;
    clearStep(motion, newLocation);
    TilePropertyWindow::invalidate(world.propertyWindow);
    onStep(newLocation);

    const uint16_t oldChunk = Chunk::chunkOfLocation(oldLocation);
    const uint16_t newChunk = Chunk::chunkOfLocation(newLocation);
    if (Chunk::chunkChanged(oldChunk, newChunk)) onChunkChange(world, newChunk);
}

#ifdef TEST
void WorldEngine::beginMoveForTest(WorldTransient &world, Direction value) {
    setDirection(world.motion, value);
    setMoving(world.motion, true);
    setWalkMask(world.motion, walkMaskFor(value));
}
#endif

void WorldEngine::onChunkChange(WorldTransient &, uint16_t newChunk) {
    // Transition-specific FX reads are attached here by their owning beads.
    (void)newChunk;
}

void WorldEngine::encounter() {
    // TODO: redesign this
}

bool WorldEngine::moveable(const WorldTransient &world) {
    const uint16_t loc = gameState.playerLocation;
    int16_t x = static_cast<uint8_t>(loc & 0xFF);
    int16_t y = static_cast<uint8_t>(loc >> 8);
    switch (direction(world.motion)) {
    case Direction::UP: --y; break;
    case Direction::DOWN: ++y; break;
    case Direction::LEFT: --x; break;
    case Direction::RIGHT: ++x; break;
    }
    return WorldCollision::canEnter(x, y, Chunk::MAP_WIDTH_TILES,
                                    Chunk::MAP_HEIGHT_TILES, world.propertyWindow);
}

void WorldEngine::interact() {
    if (!interactionJustPressedA() || interactionDialogActive()) return;

    WorldTransient &world = worldState();
    const uint16_t playerTile = gameState.playerLocation;
    uint16_t targetTile;
    if (!facedTile(playerTile, direction(world.motion), targetTile)) return;

    const uint16_t playerChunk = Chunk::chunkOfLocation(playerTile);
    if (!Chunk::validChunkId(playerChunk)) return;

    // The 128-byte script slot overlays WorldMotion. Preserve the movement
    // cursor and facing across execution; message dialogs can pause the world
    // after this call returns.
    const WorldMotion savedMotion = world.motion;
    const uint24_t slot = Chunk::scriptSlotAddr(interactionScriptsBase(), playerChunk);
    readInteractionScript(slot, world.script, Chunk::SCRIPT_SLOT_BYTES);
    runInteractionScript(world.script, playerTile, targetTile);
    world.motion = savedMotion;
}
