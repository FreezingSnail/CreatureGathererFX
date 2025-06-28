#include "World.hpp"

#include "../../common.hpp"

#include "../../globals.hpp"

#define TILE_SIZE 16
#define MAPYADJUST -3
#define MAPXADJUST -4

constexpr uint8_t tileswide = (128 / TILE_SIZE) + 4;
constexpr uint8_t tilestall = (64 / TILE_SIZE) + 2;

WorldEngine::WorldEngine() {
}
void WorldEngine::init() {
    this->mapx = 0;
    this->mapy = 0;
    this->curx = 4;
    this->cury = 2;

    up = 2;
    side = 3;
    chunkX = 4;
    chunkY = 3;
}

void WorldEngine::input() {
    if (arduboy.pressed(UP_BUTTON)) {
        this->playerDirection = Direction::UP;
        if (this->moveable()) {
            this->moving = true;
            up -= 1;
        }
    } else if (arduboy.pressed(DOWN_BUTTON)) {
        this->playerDirection = Direction::DOWN;
        if (this->moveable()) {
            this->moving = true;
            up += 1;
        }
    } else if (arduboy.pressed(LEFT_BUTTON)) {
        this->playerDirection = Direction::LEFT;
        if (this->moveable()) {
            this->moving = true;
            side -= 1;
        }
    } else if (arduboy.pressed(RIGHT_BUTTON)) {
        this->playerDirection = Direction::RIGHT;
        if (this->moveable()) {
            this->moving = true;
            side += 1;
        }
    } else {
        moving = false;
    }
}

#define PLAYER_SIZE 16
#define PLAYER_X_OFFSET WIDTH / 2 - PLAYER_SIZE / 2
#define PLAYER_Y_OFFSET HEIGHT / 2 - PLAYER_SIZE / 2

void WorldEngine::runMap() {
    // this->drawMap();

    if (this->moving && this->moveable()) {
        this->moveChar();
    } else if (!dialogMenu.peek()) {
        this->interact();
        this->input();
    } else {
        if (arduboy.justPressed(A_BUTTON)) {
            // dialogMenu.popMenu();
        }
    }

    if (up == 4 && !moving) {
        up = 0;
    } else if (up == -1 && !moving) {
        up = 3;
    }
    if (side == 4 && !moving) {
        side = 0;
    } else if (side == -1 && !moving) {
        side = 3;
    }
}

void WorldEngine::moveChar() {
    switch (this->playerDirection) {
    case Direction::UP:
        this->mapy++;
        break;
    case Direction::DOWN:
        this->mapy--;
        break;
    case Direction::LEFT:
        this->mapx++;
        break;
    case Direction::RIGHT:
        this->mapx--;
        break;
    }
    this->stepTicker++;
    if (this->stepTicker == TILE_SIZE) {
        this->stepTicker = 0;
        switch (this->playerDirection) {
        case Direction::UP:
            this->cury--;
            chunkY -= 1;
            break;
        case Direction::DOWN:
            this->cury++;
            chunkY += 1;
            break;
        case Direction::LEFT:
            this->curx--;
            chunkX -= 1;
            break;
        case Direction::RIGHT:
            this->curx++;
            chunkX += 1;
            break;
        }
        this->moving = false;
        // this->encounter(arduboy, player);
    }
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
