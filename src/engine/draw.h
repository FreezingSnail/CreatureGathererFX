#pragma once
#include "../creature/Creature.hpp"
#include "../lib/Move.hpp"
// #include "../external/Font4x6.h"
#include "../common.hpp"
#include "../globals.hpp"
#include "../lib/ReadData.hpp"
#include "../external/SpritesABC.hpp"

#include <ArduboyFX.h>

// TODO: Refactor to only 1 func call
static void printType(Type t, uint8_t x, uint8_t y) {
    switch (t) {
    case Type::SPIRIT:
        SpritesU::drawOverwriteFX(x, y, spirit, FRAME(0));
        break;
    case Type::WATER:
        SpritesU::drawOverwriteFX(x, y, water, FRAME(0));
        break;
    case Type::WIND:
        SpritesU::drawOverwriteFX(x, y, wind, FRAME(0));
        break;
    case Type::EARTH:
        SpritesU::drawOverwriteFX(x, y, earth, FRAME(0));
        break;
    case Type::FIRE:
        SpritesU::drawOverwriteFX(x, y, fire, FRAME(0));
        break;
    case Type::LIGHTNING:
        SpritesU::drawOverwriteFX(x, y, lightning, FRAME(0));
        break;
    case Type::PLANT:
        SpritesU::drawOverwriteFX(x, y, plant, FRAME(0));
        break;
    case Type::ELDER:
        SpritesU::drawOverwriteFX(x, y, elder, FRAME(0));
        break;
    case Type::STATUS:
        SpritesU::drawOverwriteFX(x, y, status, FRAME(0));
        break;
    }
}

static void setTextColorBlack() {
    FX::setFontMode(dcfWhiteBlack);
    // font.setTextColor(BLACK);
}

static void setTextColorWhite() {
    FX::setFontMode(dcmWhite);
    // font.setTextColor(WHITE);
}

static void drawInfoRec(uint8_t x, uint8_t y) {
    SpritesU::fillRect(x - 3, y - 3, 60, 30, BLACK);
    // Arduboy2::drawRect(x - 2, y - 2, 58, 28, BLACK);
    FX::drawBitmap(x - 3, y - 3, moveInfo, 0, dbmNormal);
}

static void DGF printMoveInfo(uint8_t index, uint8_t x, uint8_t y) {
    if (index == 32) {
        return;
    }
    setTextColorBlack();
    drawInfoRec(x, y);
    Move m = readMoveFX(index);

    printType(Type(m.getMoveType()), x, y);
    if (m.isPhysical()) {
        SpritesU::drawOverwriteFX(x, y + 8, physical, FRAME(0));
        // printString(font, MenuFXData::physical, x, y + 8);
    } else {
        SpritesU::drawOverwriteFX(x, y + 8, special, FRAME(0));
        // printString(font, MenuFXData::special, x, y + 8);
    }
    SpritesU::drawOverwriteFX(x, y + 16, power, FRAME(0));
    // printString(font, MenuFXData::power, x, y + 16);
    // font.setCursor(x + 30, y + 16);
    // font.print(m.power);
    drawStatNumbers(x + 33, y + 17, m.getMovePower());
}

static void printBattleMenu(int8_t index) {
}

static void printCursor(int8_t index) {
    switch (index) {
    case 0:
        FX::setCursor(0, 46);
        break;
    case 1:
        FX::setCursor(58, 46);
        break;
    case 2:
        FX::setCursor(0, 54);
        break;
    case 3:
        FX::setCursor(58, 54);
        break;
    }
    // FX::setFont(arduboyFont, dbmReverse);
    //  FX::drawChar('*');
    // FX::setFont(font4x6, dbmNormal);
}

static void printMoveMenu(int8_t index, uint8_t *moveList) {

    uint8_t color[4] = {1, 1, 1, 1};
    if (moveList[index] != 32) {
        color[index] = 0;
    }
    uint24_t rowAddress = FX::readIndexedUInt24(MoveNames::MoveNames, moveList[0]);
    SpritesU::drawOverwriteFX(6, 45, rowAddress, FRAME(color[0]));
    // printString(font, rowAddress, 6, 45);
    rowAddress = FX::readIndexedUInt24(MoveNames::MoveNames, moveList[1]);
    SpritesU::drawOverwriteFX(69, 45, rowAddress, FRAME(color[1]));
    // printString(font, rowAddress, 69, 45);
    rowAddress = FX::readIndexedUInt24(MoveNames::MoveNames, moveList[2]);
    SpritesU::drawOverwriteFX(6, 53, rowAddress, FRAME(color[2]));
    // printString(font, rowAddress, 6, 53);
    rowAddress = FX::readIndexedUInt24(MoveNames::MoveNames, moveList[3]);
    SpritesU::drawOverwriteFX(69, 53, rowAddress, FRAME(color[3]));
    // printString(font, rowAddress, 69, 53);
    printMoveInfo(moveList[index], 38, 4);
}

// TODO deal with drawing numbers
static void printCreature(Creature *creature) {
    SpritesU::drawOverwriteFX(35, 0, hpText, FRAME(0));
    SpritesU::drawOverwriteFX(35, 10, atkText, FRAME(0));
    SpritesU::drawOverwriteFX(35, 20, defText, FRAME(0));
    SpritesU::drawOverwriteFX(72, 0, satkText, FRAME(0));
    SpritesU::drawOverwriteFX(72, 10, sdefText, FRAME(0));
    SpritesU::drawOverwriteFX(72, 20, spdText, FRAME(0));

    drawStatNumbers(60, 0, creature->statlist.hp);
    drawStatNumbers(60, 10, creature->statlist.attack);
    drawStatNumbers(60, 20, creature->statlist.defense);
    drawStatNumbers(103, 0, creature->statlist.spcAtk);
    drawStatNumbers(103, 10, creature->statlist.spcDef);
    drawStatNumbers(103, 20, creature->statlist.speed);

    for (uint8_t i = 0; i < 4; i++) {
        if (creature->moves[i] == 32) {
            continue;
        }
        uint8_t offset = i % 2;
        uint8_t x = 4 + (60 * offset);
        uint8_t y = 34 + (8 * (i / 2));

        uint24_t rowAddress = FX::readIndexedUInt24(MoveNames::MoveNames, creature->moves[i]);
        SpritesU::drawOverwriteFX(x, y, rowAddress, FRAME(1));
    }
}

static void printCreatureMenu(uint8_t c1, uint8_t c2, Creature *cpointer, uint8_t index) {
    SpritesU::fillRect(0, 33, 128, 31, WHITE);
    SpritesU::fillRect(0, 0, 128, 32, BLACK);
    printCreature(cpointer);
    uint24_t c1addr = FX::readIndexedUInt24(CreatureNames::CreatureNames, c1);
    uint24_t c2addr = FX::readIndexedUInt24(CreatureNames::CreatureNames, c2);
    if (index == 0) {
        SpritesU::drawOverwriteFX(6, 49, c1addr, FRAME(0));
        SpritesU::drawOverwriteFX(6, 56, c2addr, FRAME(1));
    } else {
        SpritesU::drawOverwriteFX(6, 49, c1addr, FRAME(1));
        SpritesU::drawOverwriteFX(6, 56, c2addr, FRAME(0));
    }
}

static void drawPlayerHP(BattleEngine &engine) {
    uint16_t curHealth = engine.playerHealths[engine.playerIndex];
    uint16_t maxHealth = engine.playerCur->statlist.hp;
    double dif = static_cast<double>(curHealth) / static_cast<double>(maxHealth);
    double length = 30.0 * dif;
    SpritesU::fillRect(88, 34, 34, 6, BLACK);
    SpritesU::fillRect(90, 36, length, 2, WHITE);

    // SpritesU::fillRect(60, 38, curHealth, 2, WHITE);
    // drawStatNumbers(110, 34, curHealth);
}

static void drawOpponentHP(BattleEngine &engine) {
    uint16_t curHealth = engine.opponentHealths[engine.opponentIndex];
    uint16_t maxHealth = engine.opponentCur->statlist.hp;
    double dif = static_cast<double>(curHealth) / static_cast<double>(maxHealth);
    double length = 30.0 * dif;
    SpritesU::fillRect(6, 34, 34, 6, BLACK);
    SpritesU::fillRect(8, 36, length, 2, WHITE);
}

static void drawOpponent(BattleEngine &engine) {
    SpritesU::drawPlusMaskFX(0, 0, NewecreatureSprites, FRAME((engine.opponentCur->id * 2)));
}

static void drawPlayer(BattleEngine &engine) {
    SpritesU::drawPlusMaskFX(96, 0, NewecreatureSprites, FRAME(((engine.playerCur->id * 2) + 1)));

    drawPlayerHP(engine);
}

static void drawScene(BattleEngine &engine) {
    // SpritesU::drawPlusMaskFX(0, 15, fieldBacground, FRAME(0));
    drawPlayer(engine);
    drawOpponent(engine);
    drawOpponentHP(engine);
    drawPlayerHP(engine);
}

static void drawChunkAtOffset(uint16_t chunkIndex, int8_t offsetX, int8_t offsetY) {
    // TODO: extract tile lookup into stand alone func
    uint24_t chunkAddress = map_data + ((32 * 2) * chunkIndex);
    for (uint8_t i = 0; i < 32; i++) {
        uint8_t tileX = i % 8;   // 0-7 within chunk
        uint8_t tileY = i / 8;   // 0-3 within chunk

        // Calculate viewport tile position
        int8_t viewportTileX = offsetX + tileX;
        int8_t viewportTileY = offsetY + tileY;

        if (viewportTileX >= 0 && viewportTileX < 8 && viewportTileY >= 0 && viewportTileY < 4) {
            int8_t screenX = viewportTileX * 16;
            int8_t screenY = viewportTileY * 16;

            uint16_t tile = FX::readIndexedUInt16(chunkAddress, i);
            SpritesABC::drawSizedFX(screenX, screenY, 16, 16, tiles, SpritesABC::MODE_OVERWRITE, FRAME((tile - 1)));
        }
    }
}

static void drawMap() {
    uint16_t loc = gameState.playerLocation;

    // Convert 1D location to 2D coordinates
    uint8_t playerX = loc % 256;   // X coordinate (0-255)
    uint8_t playerY = loc / 256;   // Y coordinate (0-255)

    // Calculate the top-left corner of the 8x4 viewport in world coordinates
    // Player is at tile 3,2 of the viewport
    int16_t viewportStartX = playerX - 3;   // 3 tiles left of player
    int16_t viewportStartY = playerY - 2;   // 2 tiles above player

    // Calculate which 4 chunks we need (2x2 chunk grid for 8x4 viewport)
    uint8_t topLeftChunkX = viewportStartX / 8;
    uint8_t topLeftChunkY = viewportStartY / 4;

    // The 4 chunks we need
    struct ChunkInfo {
        uint16_t index;
        int8_t offsetX;   // Where this chunk's top-left appears in viewport tile coordinates
        int8_t offsetY;
    };

    ChunkInfo chunks[4];

    // Calculate the world coordinates of each chunk's top-left corner
    int16_t topLeftChunkWorldX = topLeftChunkX * 8;
    int16_t topLeftChunkWorldY = topLeftChunkY * 4;
    int16_t topRightChunkWorldX = (topLeftChunkX + 1) * 8;
    int16_t bottomLeftChunkWorldY = (topLeftChunkY + 1) * 4;

    // Top-left chunk
    chunks[0].index = topLeftChunkY * 32 + topLeftChunkX;
    chunks[0].offsetX = topLeftChunkWorldX - viewportStartX;
    chunks[0].offsetY = topLeftChunkWorldY - viewportStartY;

    // Top-right chunk
    chunks[1].index = topLeftChunkY * 32 + (topLeftChunkX + 1);
    chunks[1].offsetX = topRightChunkWorldX - viewportStartX;
    chunks[1].offsetY = topLeftChunkWorldY - viewportStartY;

    // Bottom-left chunk
    chunks[2].index = (topLeftChunkY + 1) * 32 + topLeftChunkX;
    chunks[2].offsetX = topLeftChunkWorldX - viewportStartX;
    chunks[2].offsetY = bottomLeftChunkWorldY - viewportStartY;

    // Bottom-right chunk
    chunks[3].index = (topLeftChunkY + 1) * 32 + (topLeftChunkX + 1);
    chunks[3].offsetX = topRightChunkWorldX - viewportStartX;
    chunks[3].offsetY = bottomLeftChunkWorldY - viewportStartY;

    // Draw all 4 chunks
    for (uint8_t i = 0; i < 4; i++) {
        // Bounds check for chunk indices
        uint8_t chunkX = chunks[i].index % 32;
        uint8_t chunkY = chunks[i].index / 32;

        if (chunkX < 32 && chunkY < 64) {
            drawChunkAtOffset(chunks[i].index, chunks[i].offsetX, chunks[i].offsetY);
        }
    }
}

// DGF static void directDrawMap() {
//     uint8_t playerX = gameState.playerLocation % 256;   // X coordinate (0-255)
//     uint8_t playerY = gameState.playerLocation / 256;   // Y coordinate (0-255)
//     uint32_t x = playerX;
//     uint32_t y = playerY;

//     uint32_t xSize = 4096;
//     uint32_t ySize = 4096 / 8;
//     FX::readDataBytes(FRAMESHIFT((worldmap + (xSize * y) + x)), arduboy.sBuffer, 128);
//     FX::readDataBytes(FRAMESHIFT((worldmap + (xSize * (y + 1)) + x)), arduboy.sBuffer + (1 * 128), 128);
//     FX::readDataBytes(FRAMESHIFT((worldmap + (xSize * (y + 2)) + x)), arduboy.sBuffer + (2 * 128), 128);
//     FX::readDataBytes(FRAMESHIFT((worldmap + (xSize * (y + 3)) + x)), arduboy.sBuffer + (3 * 128), 128);
//     FX::readDataBytes(FRAMESHIFT((worldmap + (xSize * (y + 4)) + x)), arduboy.sBuffer + (4 * 128), 128);
//     FX::readDataBytes(FRAMESHIFT((worldmap + (xSize * (y + 5)) + x)), arduboy.sBuffer + (5 * 128), 128);
//     FX::readDataBytes(FRAMESHIFT((worldmap + (xSize * (y + 6)) + x)), arduboy.sBuffer + (6 * 128), 128);
//     FX::readDataBytes(FRAMESHIFT((worldmap + (xSize * (y + 7)) + x)), arduboy.sBuffer + (7 * 128), 128);
// }
