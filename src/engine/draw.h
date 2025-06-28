#pragma once
#include "../creature/Creature.hpp"
#include "../lib/Move.hpp"
// #include "../external/Font4x6.h"
#include "../common.hpp"
#include "../globals.hpp"
#include "../lib/ReadData.hpp"
#include "../external/SpritesABC.hpp"

#include <ArduboyFX.h>
#include <stdint.h>

[[gnu::naked, gnu::noinline]]
static void fx_read_data_bytes(uint24_t addr, void *dst, size_t num) {
    // addr: r22,r23,r24
    // dst:  r20,r21
    // num:  r18,r19
    asm volatile(R"ASM(
            cbi   %[fxport], %[fxbit]
            ldi   r25, %[sfc_read]
            out   %[spdr], r25
            in    r25, %[sreg]          ;  1
            lds   r0, %[page]+0         ;  2
            add   r23, r0               ;  1
            lds   r0, %[page]+1         ;  2
            adc   r24, r0               ;  1
            rcall L%=_delay_10          ; 10
            out   %[spdr], r24
            rcall L%=_delay_17          ; 17
            out   %[spdr], r23
            rcall L%=_delay_17          ; 17
            out   %[spdr], r22
            rcall L%=_delay_16          ; 16
            out   %[spdr], r1

            ; skip straight to final read if num == 1
            movw  r26, r20              ;  1
            subi  r18, 1                ;  1
            sbci  r19, 0                ;  1
            rjmp .+0                    ;  2
            breq  2f                    ;  1 (2)
            rjmp .+0                    ;  2

            ; intermediate reads
        1:  rcall L%=_delay_7           ;  7
            cli                         ;  1
            out   %[spdr], r1           ;  1
            in    r0, %[spdr]           ;  1
            out   %[sreg], r25          ;  1
            st    X+, r0                ;  2
            subi  r18, 1                ;  1
            sbci  r19, 0                ;  1
            brne  1b                    ;  2 (1)

            ; final read
        2:  rcall L%=_delay_9           ;  9
            in    r0, %[spdr]
            st    X, r0
            sbi   %[fxport], %[fxbit]
            in    r0, %[spsr]
            ret

        L%=_delay_17:
            nop
        L%=_delay_16:
            rjmp .+0
        L%=_delay_14:
            lpm
        L%=_delay_11:
            nop
        L%=_delay_10:
            nop
        L%=_delay_9:
            rjmp .+0
        L%=_delay_7:
            ret       ; rcall is 3, ret is 4 cycles
        )ASM"
                 :
                 : [page] ""(&FX::programDataPage), [sfc_read] "I"(SFC_READ), [spdr] "I"(_SFR_IO_ADDR(SPDR)), [spsr] "I"(_SFR_IO_ADDR(SPSR)), [sreg] "I"(_SFR_IO_ADDR(SREG)),
                   [fxport] "I"(_SFR_IO_ADDR(FX_PORT)), [fxbit] "I"(FX_BIT));
}

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

static void drawMapFast() {
    uint16_t loc = gameState.playerLocation;

    // Convert 1D location to 2D coordinates
    uint8_t playerX = loc % 256;   // X coordinate (0-255)
    uint8_t playerY = loc / 256;   // Y coordinate (0-255)

    // Calculate the top-left corner of the 8x4 viewport in world coordinates
    // Player is at tile 3,2 of the viewport
    uint16_t viewportStartX = playerX - 3;   // 3 tiles left of player
    uint16_t viewportStartY = playerY - 2;   // 2 tiles above player

    uint16_t startTile = (viewportStartX + (256 * viewportStartY));

    uint8_t xOffset = xStepOffset;
    uint8_t yOffset = yStepOffset;
    uint8_t rows = 4;
    int16_t startMod = 0;
    int8_t xShift = 0;
    int8_t yShift = 0;
    if (walkingMask != 0) {
        switch (walkingMask) {
        case 0b10000000:
            startMod = -1 * 2;
            xShift = 1;
            break;
        case 0b01000000:
            // do nothing
            break;
        case 0b00100000:
            rows = 5;
            startMod = -256 * 2;
            yShift = -1;
            break;
        case 0b00001000:
            rows = 5;
            break;
        }
    }

    // need to double the tile since uint16_t
    uint24_t intialAdder = raw_map_data + (startTile * 2);
    for (uint8_t i = 0; i < rows; i++) {
        // uint16_t rowbuf[9];
        uint24_t addr = intialAdder + (i * 512) + startMod;
        fx_read_data_bytes(addr, rowbuf, sizeof(rowbuf));
        for (uint8_t j = 0; j < 9; j++) {
            uint8_t screenX = (16 * j);
            uint8_t screenY = (16 * i);
            uint16_t tile = rowbuf[j] - 1;
            int8_t x = screenX + xOffset;
            int8_t y = screenY + yOffset;

            if (xShift == 1) {
                x -= 16;
            } else if (yShift == -1) {
                y -= 16;
            }
            SpritesABC ::drawSizedFX(x, y, 16, 16, tiles, SpritesABC::MODE_OVERWRITE, FRAME(tile));
        }
    }
}
