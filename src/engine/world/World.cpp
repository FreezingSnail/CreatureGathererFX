#include "World.hpp"
#include "WorldCollision.hpp"
#include "StepEvent.hpp"
#include "Encounter.hpp"
#include "LureZone.hpp"
#include "TileProps.hpp"
#include "../../item/Inventory.hpp"
#include "../../item/ItemIds.hpp"
#include "../../lib/FxRead.hpp"
#include "../../fxdata.h"

#ifndef TEST
#include "../../common.hpp"
#include "../../globals.hpp"
#else
#include <Arduboy2.h>
#include "../../GameState.hpp"
extern GameState gameState;
extern bool worldInteractionJustPressedA();
extern bool worldInteractionDialogActive();
extern void worldInteractionPopDialog();
extern void worldInteractionReadScript(uint24_t address, uint8_t *buffer, uint8_t length);
extern void worldInteractionRunScript(uint8_t *script, uint16_t currentTile, uint16_t targetTile);
extern uint24_t worldInteractionScriptsBase();
extern uint8_t worldMovementButtons();
extern uint8_t worldPickerButtons();
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

__attribute__((noinline)) void clearStep(WorldMotion &motion, uint16_t newOrigin) {
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
#elif defined(CGFX_FULL_WORLD_VM)
    return dialogMenu.peek();
#else
    return false;
#endif
}

void popInteractionDialog() {
#ifdef TEST
    worldInteractionPopDialog();
#elif defined(CGFX_FULL_WORLD_VM)
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
#ifdef CGFX_FULL_WORLD_VM
    vm.initVM(script, currentTile, targetTile);
    vm.run();
#else
    vm.initWarpVM(script, currentTile, targetTile);
    vm.runWarpVM();
#endif
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

uint8_t movementButtons() {
#ifdef TEST
    return worldMovementButtons();
#else
    uint8_t buttons = 0;
    if (arduboy.pressed(LEFT_BUTTON)) buttons |= LEFT_BUTTON;
    if (arduboy.pressed(RIGHT_BUTTON)) buttons |= RIGHT_BUTTON;
    if (arduboy.pressed(UP_BUTTON)) buttons |= UP_BUTTON;
    if (arduboy.pressed(DOWN_BUTTON)) buttons |= DOWN_BUTTON;
    return buttons;
#endif
}

uint8_t pickerButtons() {
#ifdef TEST
    return worldPickerButtons();
#else
    uint8_t buttons = 0;
    if (arduboy.justPressed(UP_BUTTON)) buttons |= LureZone::PICKER_BTN_UP;
    if (arduboy.justPressed(DOWN_BUTTON)) buttons |= LureZone::PICKER_BTN_DOWN;
    if (arduboy.justPressed(A_BUTTON)) buttons |= LureZone::PICKER_BTN_A;
    if (arduboy.justPressed(B_BUTTON)) buttons |= LureZone::PICKER_BTN_B;
    return buttons;
#endif
}

// The pocket box (tsx id 544) carries PROP_SPARE and the plant (tsx id 550)
// carries PROP_PLANTABLE; both are read from the RAM property window so the
// A-press hooks never add FX reads on top of the single VM slot read.
}

void WorldEngine::init(WorldTransient &world) {
    world.activateMotion();
    TilePropertyWindow::invalidate(world.propertyWindow);
    Encounter::invalidate(world.zoneTableCache);
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
    onChunkChange(world, Chunk::chunkOfLocation(gameState.playerLocation));
}

void WorldEngine::setPos(WorldTransient &world, uint8_t x, uint8_t y) {
    const uint16_t oldChunk = Chunk::chunkOfLocation(gameState.playerLocation);
    gameState.playerLocation = static_cast<uint16_t>(x) |
                               (static_cast<uint16_t>(y) << 8);
    clearStep(world.motion, gameState.playerLocation);
    TilePropertyWindow::invalidate(world.propertyWindow);
    const uint16_t newChunk = Chunk::chunkOfLocation(gameState.playerLocation);
    if (Chunk::chunkChanged(oldChunk, newChunk)) onChunkChange(world, newChunk);
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

uint8_t WorldEngine::playerFrame(const WorldTransient &world) {
    const WorldMotion &motion = world.motion;
    const uint8_t base = static_cast<uint8_t>(direction(motion)) * 3;
    return base + (motion.step == 0 ? 0 : motion.step < 8 ? 1 : 2);
}

void WorldEngine::syncFromLocation(WorldTransient &world) {
    const uint16_t published = gameState.playerLocation;
    if (published == origin(world.motion)) return;
    const uint16_t oldChunk = Chunk::chunkOfLocation(origin(world.motion));
    clearStep(world.motion, published);
    TilePropertyWindow::invalidate(world.propertyWindow);
    const uint16_t newChunk = Chunk::chunkOfLocation(published);
    if (Chunk::chunkChanged(oldChunk, newChunk)) onChunkChange(world, newChunk);
}

void WorldEngine::input(WorldTransient &world) {
    inputFromButtons(world, movementButtons());
}

void WorldEngine::inputFromButtons(WorldTransient &world, uint8_t buttons) {
    Direction requested;
    bool hasRequest = true;
    if (buttons & LEFT_BUTTON) {
        requested = Direction::LEFT;
    } else if (buttons & RIGHT_BUTTON) {
        requested = Direction::RIGHT;
    } else if (buttons & UP_BUTTON) {
        requested = Direction::UP;
    } else if (buttons & DOWN_BUTTON) {
        requested = Direction::DOWN;
    } else {
        hasRequest = false;
    }

    WorldMotion &motion = world.motion;
    if (!hasRequest) {
        setMoving(motion, false);
        return;
    }
    setDirection(motion, requested);
    if (moveable(world)) {
        setMoving(motion, true);
        setWalkMask(motion, walkMaskFor(requested));
    }
}

void WorldEngine::runMap(WorldTransient &world) {
    syncFromLocation(world);
    // The boot intro card freezes the world; any fresh button edge dismisses it.
    if (LureZone::introActive()) {
        LureZone::introInput(pickerButtons());
        return;
    }
    // A deposited material picker freezes world movement while it is open.
    if (LureZone::pickerOpen()) {
        LureZone::pickerInput(pickerButtons());
        return;
    }
    if (interactionDialogActive()) {
        if (interactionJustPressedA()) popInteractionDialog();
    } else if (moving(world.motion) && moveable(world)) {
        moveChar(world);
    } else {
        const uint16_t beforeInteract = gameState.playerLocation;
        interact();
        if (gameState.playerLocation != beforeInteract) syncFromLocation(world);
        if (!interactionDialogActive() && !LureZone::pickerOpen()) input(world);
        // Match the previous sketch path: the first animation pixel is applied
        // on the same frame that accepts the direction, for 16 ticks per tile.
        if (!interactionDialogActive() && !LureZone::pickerOpen() &&
            moving(world.motion) && moveable(world)) {
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

    const uint16_t oldChunk = Chunk::chunkOfLocation(oldLocation);
    const uint16_t newChunk = Chunk::chunkOfLocation(newLocation);
    if (Chunk::chunkChanged(oldChunk, newChunk)) onChunkChange(world, newChunk);

    // The destination property window remains readable during dispatch. It is
    // invalidated only after the hook because encounter gating is RAM-only.
    onStep(newLocation);
    if (gameState.state == GameState_t::WORLD) {
        TilePropertyWindow::invalidate(world.propertyWindow);
    }
}

#ifdef TEST
void WorldEngine::beginMoveForTest(WorldTransient &world, Direction value) {
    setDirection(world.motion, value);
    setMoving(world.motion, true);
    setWalkMask(world.motion, walkMaskFor(value));
}
#endif

void WorldEngine::onChunkChange(WorldTransient &world, uint16_t newChunk) {
    // A lure belongs to the authored zones, not one chunk: crossing between the
    // pocket's chunk 1 and chunk 33 keeps it, while entering a chunk with no
    // authored zone (load() returns false) clears it before the new zone's
    // encounter table takes over.
    if (!Chunk::validChunkId(newChunk)) {
        Encounter::invalidate(world.zoneTableCache);
        LureZone::clear();
        return;
    }
    if (!Encounter::load(world.zoneTableCache, newChunk)) LureZone::clear();
}

void WorldEngine::encounter() {
    // Wild rolls belong to onStep(), which receives the committed destination.
    // Keep this legacy symbol inert so callers cannot create a second roll path.
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

    // Faced-tile props come from the RAM property window (the same storage as
    // collision), so the box and plant hooks add no FX reads on the A press.
    uint8_t targetProps = 0;
    bool targetOccupied = false;
    const int16_t targetX = static_cast<uint8_t>(targetTile);
    const int16_t targetY = static_cast<uint8_t>(targetTile >> 8);
    if (TilePropertyWindow(world.propertyWindow)
            .lookup(targetX, targetY, targetProps, targetOccupied) &&
        targetOccupied) {
        if ((targetProps & TileProps::PROP_SPARE) != 0) {
            LureZone::openPicker();
            return;
        }
        if ((targetProps & TileProps::PROP_PLANTABLE) != 0) {
            item::inventoryAdd(LureZone::inventory(), item::ItemKind::Lure,
                               item::lureId(0, 0), 1);
            LureZone::showMessage(lureTextGathered, 45, 40);
            return;
        }
    }

    // The 128-byte script slot overlays WorldMotion. Preserve the movement
    // cursor and facing across execution; message dialogs can pause the world
    // after this call returns.
    const WorldMotion savedMotion = world.motion;
    const uint24_t slot = Chunk::scriptSlotAddr(interactionScriptsBase(), playerChunk);
    readInteractionScript(slot, world.script, Chunk::SCRIPT_SLOT_BYTES);
    runInteractionScript(world.script, playerTile, targetTile);
    world.motion = savedMotion;
}
