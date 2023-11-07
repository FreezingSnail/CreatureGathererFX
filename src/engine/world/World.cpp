#include "World.hpp"
#include <ArduboyFX.h>

#include "../../fxdata/fxdata.h"
// #include "../../sprites/characterSheet.h"
#include "../battle/Battle.hpp"
#include "../game/Gamestate.hpp"
#include "Encounter.hpp"
#include "Map.hpp"

#define TILE_SIZE 16
constexpr uint8_t tileswide = (128 / TILE_SIZE) + 4;
constexpr uint8_t tilestall = (64 / TILE_SIZE) + 4;

bool __attribute__((optimize("-O0"))) warpTile(uint8_t x, uint8_t y) {
    uint8_t tile = gameMap[y][x];
    if (tile == 26 || tile == 28)
        return true;

    return false;
}

WorldEngine::WorldEngine() {}
void WorldEngine::init(Arduboy2 *arduboy, GameState_t *state, BattleEngine *battleEngine, MenuV2 *menu2) {
    this->arduboy = arduboy;
    this->encounterTable = Encounter(arduboy);
    this->battleEngine = battleEngine;
    this->state = state;
    this->menu2 = menu2;
    this->mapx = 0;
    this->mapy = 0;
    this->curx = 4;
    this->cury = 2;
    this->loadMap(0, 1);
}

void __attribute__((optimize("-O0"))) WorldEngine::loadMap(uint8_t mapIndex, uint8_t submapIndex) {
    this->mapIndex = mapIndex;
    // warp zones
    uint24_t warpsAddress = FX::readIndexedUInt24(MapData::warps, mapIndex);
    warpsAddress = FX::readIndexedUInt24(warpsAddress, submapIndex);
    FX::readDataObject(warpsAddress, this->warps);

    // map data
    // submap count
    uint24_t address = MapData::maps + ((sizeof(uint24_t) * 5) * mapIndex);
    FX::readDataObject(FX::readIndexedUInt24(address, 0), this->submapCount);

    // map dims
    uint24_t widthAddress = FX::readIndexedUInt24(address, 4);
    uint24_t heightsAddress = FX::readIndexedUInt24(address, 5);
    FX::readDataObject(widthAddress + sizeof(uint8_t) * submapIndex, this->width);
    FX::readDataObject(heightsAddress + sizeof(uint8_t) * submapIndex, this->height);

    // map address
    uint24_t mapAddress;
    if (submapIndex == 0) {
        mapAddress = FX::readIndexedUInt24(address, 1);

    } else {
        mapAddress = FX::readIndexedUInt24(address, 2);
        mapAddress = FX::readIndexedUInt24(mapAddress, submapIndex - 1);
    }

    for (uint8_t i = 0; i < this->height; i++) {
        uint24_t offset = (sizeof(uint8_t) * this->width) * i;
        FX::readDataBytes(mapAddress + offset, gameMap[i], sizeof(uint8_t) * this->width);
    }

    for (uint8_t i = 0; i < 6; i++) {
        this->events[i].loadEvent(mapIndex, submapIndex, i);
    }
}

void WorldEngine::drawMap() {
    for (uint8_t y = 0; y < tilestall; y++) {
        for (uint8_t x = 0; x < tileswide; x++) {
            const uint8_t tilex = x - this->mapx / TILE_SIZE;
            const uint8_t tiley = y - this->mapy / TILE_SIZE;
            if (tilex >= 0 && tiley >= 0 && tilex < this->width && tiley < this->height) {
                FX::drawBitmap(x * TILE_SIZE + this->mapx % TILE_SIZE - 9, y * TILE_SIZE + this->mapy % TILE_SIZE - 8, tilesheet,
                               gameMap[tiley][tilex], dbmNormal);
            }
        }
    }
}
constexpr int8_t EVENT_X_OFFSET = WIDTH / 2;
constexpr int8_t EVENT_Y_OFFSET = HEIGHT / 2;

void WorldEngine::drawEvents() {
    // mapx -> 0 @ 4 -> n*-16 + 64 offset
    // mapy 0 @ 2 -> n*-16 + 32 offset
    for (uint8_t i = 0; i < 6; i++) {
        Event *e = &this->events[i];
        if (e->cords.x < (this->curx + 6) && e->cords.x > (this->curx - 6) && e->cords.y < (this->cury + 3) &&
            e->cords.y > (this->cury - 3)) {
            // draw event
            int16_t xOffset = e->cords.x - this->curx;
            int16_t yOffset = e->cords.y - this->cury;
            int xMod = 0;
            int yMod = 0;
            if (this->playerDirection == Up) {
                yMod = 1;
            } else if (this->playerDirection == Down) {
                yMod = -1;
            } else if (this->playerDirection == Left) {
                xMod = 1;
            } else if (this->playerDirection == Right) {
                xMod = -1;
            }

            uint16_t x = EVENT_X_OFFSET - 9 + (xOffset * TILE_SIZE) + (this->stepTicker % TILE_SIZE) * xMod;
            uint16_t y = EVENT_Y_OFFSET - 8 + (yOffset * TILE_SIZE) + (this->stepTicker % TILE_SIZE) * yMod;
            e->draw(x, y);
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
    } else {
        moving = false;
    }
}

#define PLAYER_SIZE 16
#define PLAYER_X_OFFSET WIDTH / 2 - PLAYER_SIZE / 2
#define PLAYER_Y_OFFSET HEIGHT / 2 - PLAYER_SIZE / 2
void WorldEngine::drawPlayer() {
    uint8_t frame = ((int)(this->playerDirection) * 3) + ((this->stepTicker - 1) / 5);
    FX::drawBitmap(PLAYER_X_OFFSET, PLAYER_Y_OFFSET, characterSheet, frame, dbmNormal);
}

void __attribute__((optimize("-O0"))) WorldEngine::runMap(Player *player, Menu *menu) {
    this->drawMap();
    this->drawPlayer();
    this->drawEvents();

    if (this->moving && this->moveable()) {
        this->moveChar(player, menu);
    } else if (!this->menu2->drawPopMenu()) {
        this->interact();
        this->input();
    } else {
        if (this->arduboy->justPressed(A_BUTTON)) {
            this->menu2->popMenu();
        }
    }
}

void WorldEngine::moveChar(Player *player, Menu *menu) {
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
        this->moving = false;
        if (warpTile(this->curx, this->cury)) {
            this->warp();
        }
        this->encounter(this->arduboy, player, menu);
    }
}

uint8_t WorldEngine::getTile() { return (TileType)0; }

void __attribute__((optimize("-O0"))) WorldEngine::encounter(Arduboy2 *arduboy, Player *player, Menu *menu) {
    uint8_t t = gameMap[this->cury][this->curx];
    if (t == 0) {
        uint8_t chance = random(1, 101);
        if (chance <= 10) {
            uint8_t creatureID = this->encounterTable.rollEncounter();
            uint8_t level = this->encounterTable.rollLevel();
            this->battleEngine->startEncounter(*this->arduboy, *player, *menu, creatureID, level);
            *this->state = GameState_t::BATTLE;
        }
    }
}

bool __attribute__((optimize("-O0"))) WorldEngine::moveable() {
    uint8_t tilex = this->curx;
    uint8_t tiley = this->cury;
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

    if (tilex < 0 || tiley < 0 || tilex >= this->width || tiley >= this->height) {
        return false;
    }

    for (uint8_t i = 0; i < EVENTCOUNT; i++) {
        Event e = this->events[i];
        if (e.cords.x == tilex && e.cords.y == tiley)
            return false;
    }
    this->nextTile = gameMap[tiley][tilex];
    switch (this->nextTile) {
    case TREES:
    case GRASS:
        return true;

    default:
        return true;
    }
}

void __attribute__((optimize("-O0"))) WorldEngine::interact() {
    if (this->arduboy->justPressed(A_BUTTON)) {
        uint8_t tilex = this->curx;
        uint8_t tiley = this->cury;
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
        for (uint8_t i = 0; i < EVENTCOUNT; i++) {
            Event e = this->events[i];
            if (e.cords.x == tilex && e.cords.y == tiley) {
                this->menu2->pushEvent(e);
                return;
            }
        }
    }
}

void WorldEngine::warp() {
    for (uint8_t i = 0; i < 6; i++) {
        if (this->curx == this->warps[i][1] && this->cury == this->warps[i][0]) {
            this->loadMap(this->warps[i][2], this->warps[i][3]);
            this->setPos(warps[i][1], warps[i][0]);
            return;
        }
    }
}

void __attribute__((optimize("-O0"))) WorldEngine::setPos(uint8_t x, uint8_t y) {
    this->curx = x;
    this->cury = y;
    int8_t xdif = 4 - x;
    int8_t ydif = 2 - y;
    this->mapx = xdif * 16;
    this->mapy = ydif * 16;
}