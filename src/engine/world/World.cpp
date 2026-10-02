#include "World.hpp"

#ifndef TEST
#include "../../common.hpp"
#include "../../globals.hpp"
#else
#include "../../GameState.hpp"
extern GameState gameState;
#endif

#define TILE_SIZE 16
#define MAPYADJUST -3
#define MAPXADJUST -4

constexpr uint8_t tileswide = (128 / TILE_SIZE) + 4;
constexpr uint8_t tilestall = (64 / TILE_SIZE) + 2;

void WorldEngine::init() {
    this->playerDirection = Direction::DOWN;
    this->width = this->height = 0;
    this->stepOffsetX = 0;
    this->stepOffsetY = 0;
    this->walkMask = 0;
    this->moving = false;
    this->stepTicker = 0;
    this->curx = 4;
    this->cury = 2;
    gameState.playerLocation = static_cast<uint16_t>(curx) +
                               (static_cast<uint16_t>(cury) << 8);
    lastChunk = Chunk::chunkAt(static_cast<uint8_t>(curx), static_cast<uint8_t>(cury));
}

void WorldEngine::loadMap(uint8_t mapIndex, uint8_t submapIndex) {
    // The FX image contains one canonical map; it occupies map/submap 0/0.
    (void)mapIndex;
    (void)submapIndex;
    this->width = 32;
    this->height = 16;
}

void WorldEngine::setPos(uint8_t x, uint8_t y) {
    this->curx = x;
    this->cury = y;
    gameState.playerLocation = static_cast<uint16_t>(x) +
                               (static_cast<uint16_t>(y) << 8);
    this->stepOffsetX = this->stepOffsetY = 0;
    this->walkMask = 0;
    this->moving = false;
    this->stepTicker = 0;
    lastChunk = Chunk::chunkOfLocation(gameState.playerLocation);
}

uint16_t WorldEngine::location() const {
    return gameState.playerLocation;
}

ViewOffset WorldEngine::view() const {
    ViewOffset result = {stepOffsetX, stepOffsetY, walkMask};
    return result;
}

void WorldEngine::syncFromLocation() {
    const uint16_t published = gameState.playerLocation;
    const uint8_t x = static_cast<uint8_t>(published & 0xFF);
    const uint8_t y = static_cast<uint8_t>(published >> 8);
    if (x == curx && y == cury) return;

    curx = x;
    cury = y;
    stepOffsetX = stepOffsetY = 0;
    walkMask = 0;
    moving = false;
    stepTicker = 0;
    lastChunk = Chunk::chunkOfLocation(published);
}

void WorldEngine::input() {
#ifdef TEST
    // Device input is covered by the Arduboy path; host tests drive movement
    // through beginMoveForTest() so they do not need Arduino headers.
#else
    if (arduboy.pressed(LEFT_BUTTON)) {
        this->playerDirection = Direction::LEFT;
        if (this->moveable()) {
            this->moving = true;
            this->walkMask = 0b10000000;
        }
    } else if (arduboy.pressed(RIGHT_BUTTON)) {
        this->playerDirection = Direction::RIGHT;
        if (this->moveable()) {
            this->moving = true;
            this->walkMask = 0b01000000;
        }
    } else if (arduboy.pressed(UP_BUTTON)) {
        this->playerDirection = Direction::UP;
        if (this->moveable()) {
            this->moving = true;
            this->walkMask = 0b00100000;
        }
    } else if (arduboy.pressed(DOWN_BUTTON)) {
        this->playerDirection = Direction::DOWN;
        if (this->moveable()) {
            this->moving = true;
            this->walkMask = 0b00001000;
        }
    } else {
        moving = false;
    }
#endif
}

#define PLAYER_SIZE 16
#define PLAYER_X_OFFSET WIDTH / 2 - PLAYER_SIZE / 2
#define PLAYER_Y_OFFSET HEIGHT / 2 - PLAYER_SIZE / 2

void WorldEngine::runMap() {
#ifdef TEST
    syncFromLocation();
#else
    this->syncFromLocation();

    if (this->moving && this->moveable()) {
        this->moveChar();
    } else if (!dialogMenu.peek()) {
        this->interact();
        this->input();
        // Match the old sketch path: the first animation pixel is applied on
        // the same frame that accepts the direction, for 16 ticks per tile.
        if (this->moving && this->moveable()) this->moveChar();
    } else {
        if (arduboy.justPressed(A_BUTTON)) {
            // dialogMenu.popMenu();
        }
    }
#endif

}

void WorldEngine::moveChar() {
    switch (this->playerDirection) {
    case Direction::UP:
        this->stepOffsetY++;
        break;
    case Direction::DOWN:
        this->stepOffsetY--;
        break;
    case Direction::LEFT:
        this->stepOffsetX++;
        break;
    case Direction::RIGHT:
        this->stepOffsetX--;
        break;
    }
    this->stepTicker++;
    if (this->stepTicker == TILE_SIZE) {
        this->stepTicker = 0;
        switch (this->playerDirection) {
        case Direction::UP:
            this->cury--;
            break;
        case Direction::DOWN:
            this->cury++;
            break;
        case Direction::LEFT:
            this->curx--;
            break;
        case Direction::RIGHT:
            this->curx++;
            break;
        }
        gameState.playerLocation = static_cast<uint16_t>(curx) +
                                   (static_cast<uint16_t>(cury) << 8);
        const uint16_t newChunk = Chunk::chunkOfLocation(gameState.playerLocation);
        if (Chunk::chunkChanged(lastChunk, newChunk)) onChunkChange(newChunk);
        this->moving = false;
        this->walkMask = 0;
        this->stepOffsetX = this->stepOffsetY = 0;
    }
}

#ifdef TEST
void WorldEngine::beginMoveForTest(Direction direction) {
    playerDirection = direction;
    moving = true;
    switch (direction) {
    case Direction::LEFT: walkMask = 0b10000000; break;
    case Direction::RIGHT: walkMask = 0b01000000; break;
    case Direction::UP: walkMask = 0b00100000; break;
    case Direction::DOWN: walkMask = 0b00001000; break;
    }
}
#endif

void WorldEngine::onChunkChange(uint16_t newChunk) {
    // Transition-specific FX reads are attached here by their owning beads.
    lastChunk = newChunk;
}

void WorldEngine::encounter() {
    // TODO: redesign this
}

bool WorldEngine::moveable() {
    uint8_t tilex = this->curx;
    uint8_t tiley = this->cury;
    switch (this->playerDirection) {
    case Direction::UP:
        tiley--;
        break;
    case Direction::DOWN:
        tiley++;
        break;
    case Direction::LEFT:
        tilex--;
        break;
    case Direction::RIGHT:
        tilex++;
        break;
    }

    if (tilex < 0 || tiley < 0 || tilex >= this->width || tiley >= this->height) {
        return false;
    }

    // TODO: need to do a tile lookup

    return true;
}

void WorldEngine::interact() {
    // TODO: move to vm
}
