#pragma once
#include "../creature/Creature.hpp"
#include "../lib/Move.hpp"
// #include "../external/Font4x6.h"
#include "../common.hpp"
#include "../lib/Text.hpp"
#include "../globals.hpp"
#include "../lib/ReadData.hpp"
#include "../lib/FxRead.hpp"
#include "world/Chunk.hpp"
#include "world/TilePropertyWindow.hpp"
#include "world/World.hpp"
#include "battle/BattleSession.hpp"
#include "battle/BattleView.hpp"

#include <ArduboyFX.h>
#include <stdint.h>

[[gnu::naked, gnu::noinline]]
static void fx_read_data_bytes_raw(uint24_t addr, void *dst, size_t num) {
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

static void fx_read_data_bytes(uint24_t addr, void *dst, size_t num) {
    FxReadCounter::record();
    fx_read_data_bytes_raw(addr, dst, num);
}

// TODO: Refactor to only 1 func call
static void printType(Type t, uint8_t x, uint8_t y) {
    switch (t) {
    case Type::SPIRIT:
        drawText(x, y, spirit, 35, FRAME(0));
        break;
    case Type::WATER:
        drawText(x, y, water, 30, FRAME(0));
        break;
    case Type::WIND:
        drawText(x, y, wind, 25, FRAME(0));
        break;
    case Type::EARTH:
        drawText(x, y, earth, 30, FRAME(0));
        break;
    case Type::FIRE:
        drawText(x, y, fire, 25, FRAME(0));
        break;
    case Type::LIGHTNING:
        drawText(x, y, lightning, 50, FRAME(0));
        break;
    case Type::PLANT:
        drawText(x, y, plant, 30, FRAME(0));
        break;
    case Type::ELDER:
        drawText(x, y, elder, 30, FRAME(0));
        break;
    case Type::STATUS:
        drawText(x, y, status, 35, FRAME(0));
        break;
    }
}

static void drawInfoRec(uint8_t x, uint8_t y) {
    Blit::fillRect(x - 3, y - 3, 60, 30, BLACK);
    // Arduboy2::drawRect(x - 2, y - 2, 58, 28, BLACK);
}

static uint16_t packedMoveInfo(const uint8_t *packed, uint8_t slot) {
    if (packed == nullptr || slot >= 4) {
        return 0;
    }
    const uint8_t bitOffset = static_cast<uint8_t>(slot * 10);
    uint16_t info = 0;
    for (uint8_t bit = 0; bit < 10; ++bit) {
        const uint8_t offset = static_cast<uint8_t>(bitOffset + bit);
        if ((packed[offset >> 3] & (1u << (offset & 7))) != 0) {
            info |= static_cast<uint16_t>(1u << bit);
        }
    }
    return info;
}

static void printMoveInfoValues(uint8_t type, bool isPhysical,
                                uint8_t movePower, uint8_t x, uint8_t y) {
    drawInfoRec(x, y);
    printType(Type(type), x, y);
    if (isPhysical) {
        drawText(x, y + 8, physical, 25, FRAME(0));
    } else {
        drawText(x, y + 8, special, 25, FRAME(0));
    }
    drawText(x, y + 16, power, 35, FRAME(0));
    drawStatNumbers(x + 33, y + 17, movePower);
}

static void printMoveInfo(uint8_t index, uint8_t x, uint8_t y, Move m) {
    if (index == 32) {
        return;
    }
    printMoveInfoValues(m.getMoveType(), m.isPhysical(), m.getMovePower(), x, y);
}

static void printPackedMoveInfo(uint8_t moveId, uint8_t slot, uint8_t x,
                                uint8_t y, const uint8_t *packed) {
    if (moveId >= 32 || slot >= 4) {
        return;
    }
    const uint16_t info = packedMoveInfo(packed, slot);
    printMoveInfoValues(static_cast<uint8_t>((info >> 6) & 0x0f),
                        (info & 1u) != 0,
                        static_cast<uint8_t>((info >> 1) & 0x1f), x, y);
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

static void printMoveMenu(int8_t index, const battle::MoveSnapshot &moves,
                          const uint24_t *nameAddresses,
                          const uint8_t *moveInfo) {
    if (nameAddresses == nullptr || moveInfo == nullptr) {
        return;
    }

    uint8_t selected = index < 0 ? 0 : static_cast<uint8_t>(index);
    if (selected >= 4) {
        selected = 3;
    }
    uint8_t color[4] = {1, 1, 1, 1};
    if (moves.moveIds[selected] < 32) {
        color[selected] = 0;
    }
    drawText(6, 45, nameAddresses[0],
                     readMoveNameWidth(moves.moveIds[0]), FRAME(color[0]));
    drawText(69, 45, nameAddresses[1],
                     readMoveNameWidth(moves.moveIds[1]), FRAME(color[1]));
    drawText(6, 53, nameAddresses[2],
                     readMoveNameWidth(moves.moveIds[2]), FRAME(color[2]));
    drawText(69, 53, nameAddresses[3],
                     readMoveNameWidth(moves.moveIds[3]), FRAME(color[3]));
    printPackedMoveInfo(moves.moveIds[selected], selected, 38, 4, moveInfo);
}

static void printCreatureMenu(const battle::PartySnapshot &party, uint8_t index,
                              const uint24_t *creatureNames) {
    Blit::fillRect(0, 33, 128, 31, WHITE);
    Blit::fillRect(0, 0, 128, 32, BLACK);
    if (creatureNames == nullptr || party.count == 0) {
        return;
    }

    const uint8_t count = party.count > 2 ? 2 : party.count;
    const uint8_t selected = index < count ? index : 0;
    for (uint8_t row = 0; row < count; ++row) {
        const uint8_t id = party.choices[row].id;
        drawText(6, static_cast<uint8_t>(49 + row * 7),
                         creatureNames[row], readCreatureNameWidth(id),
                         FRAME(row == selected ? 0 : 1));
    }
}


static uint8_t battleHpBarWidth(uint8_t hp, uint8_t maxHp) {
    if (maxHp == 0) {
        return 0;
    }
    const uint8_t boundedHp = hp > maxHp ? maxHp : hp;
    return static_cast<uint8_t>((static_cast<uint16_t>(boundedHp) * 30) / maxHp);
}

static void drawPlayerHP(const battle::BattleView &view) {
    const battle::ActiveView &creature = view.active[static_cast<uint8_t>(battle::Side::Player)];
    const uint8_t hp = creature.hp;
    Blit::fillRect(88, 34, 34, 6, BLACK);
    Blit::fillRect(90, 36, battleHpBarWidth(hp, creature.maxHp), 2, WHITE);

    // Blit::fillRect(60, 38, curHealth, 2, WHITE);
    // drawStatNumbers(110, 34, curHealth);
}

static void drawOpponentHP(const battle::BattleView &view) {
    const battle::ActiveView &creature = view.active[static_cast<uint8_t>(battle::Side::Opponent)];
    Blit::fillRect(6, 34, 34, 6, BLACK);
    Blit::fillRect(8, 36, battleHpBarWidth(creature.hp, creature.maxHp), 2, WHITE);
}

static constexpr uint8_t kBattleSpeciesCount = 32;

static void clearBattleSprite(uint8_t x) {
    // Creature art is masked, so transparent pixels do not replace pixels
    // left by the outgoing creature when a switch redraws the scene.
    Blit::fillRect(x, 0, 32, 32, BLACK);
}

static void drawOpponent(const battle::BattleView &view) {
    const battle::ActiveView &creature = view.active[static_cast<uint8_t>(battle::Side::Opponent)];
    clearBattleSprite(0);
    if (creature.id >= kBattleSpeciesCount) {
        return;
    }
    Blit::draw(0, 0, 32, 32, NewecreatureSprites, FRAME((creature.id * 2)), Blit::PLUSMASK);
}

static void drawPlayer(const battle::BattleView &view) {
    const battle::ActiveView &creature = view.active[static_cast<uint8_t>(battle::Side::Player)];
    clearBattleSprite(96);
    if (creature.id < kBattleSpeciesCount) {
        Blit::draw(96, 0, 32, 32, NewecreatureSprites,
                                FRAME(((creature.id * 2) + 1)), Blit::PLUSMASK);
    }

    drawPlayerHP(view);
}

static void drawScene(const battle::BattleView &view) {
    // Blit::draw(0, 15, fieldBacground, FRAME(0), Blit::PLUSMASK);
    drawPlayer(view);
    drawOpponent(view);
    drawOpponentHP(view);
    drawPlayerHP(view);
}

static uint8_t drawMapFast(WorldTransient &world) {
    const uint16_t loc = WorldEngine::location();
    const ViewOffset view = WorldEngine::view(world);

    // Convert 1D location to 2D coordinates
    const int16_t playerX = loc & 0xFF;   // X coordinate (0-255)
    const int16_t playerY = loc >> 8;    // Y coordinate (0-255)

    // Calculate the top-left corner of the 8x4 viewport in world coordinates
    // Player is at tile 3,2 of the viewport
    const int16_t viewportStartX = playerX - 3;   // 3 tiles left of player
    const int16_t viewportStartY = playerY - 2;   // 2 tiles above player

    const int8_t xOffset = view.x;
    const int8_t yOffset = view.y;
    uint8_t rows = 4;
    int16_t readStartX = viewportStartX;
    int16_t readStartY = viewportStartY;
    int8_t xShift = 0;
    int8_t yShift = 0;
    if (view.mask != 0) {
        switch (view.mask) {
        case 0b10000000:
            --readStartX;
            xShift = 1;
            break;
        case 0b01000000:
            // do nothing
            break;
        case 0b00100000:
            rows = 5;
            --readStartY;
            yShift = -1;
            break;
        case 0b00001000:
            rows = 5;
            break;
        }
    }

    TilePropertyWindow propertyWindow(world.propertyWindow);
    propertyWindow.begin(readStartX, readStartY);
    uint8_t rowReads = 0;
    // TODO: need to lock rows to 2 when text is drawn
    for (uint8_t i = 0; i < rows; i++) {
        uint16_t rowbuf[TilePropertyWindow::WINDOW_WIDTH] = {};
        const int16_t mapY = readStartY + i;
        const int16_t firstMapX = readStartX < 0 ? 0 : readStartX;
        const int16_t endMapX = readStartX + TilePropertyWindow::WINDOW_WIDTH > 256
                                    ? 256
                                    : readStartX + TilePropertyWindow::WINDOW_WIDTH;
        if (mapY >= 0 && mapY < 256 && firstMapX < endMapX) {
            const uint8_t firstColumn = static_cast<uint8_t>(firstMapX - readStartX);
            const uint8_t cellCount = static_cast<uint8_t>(endMapX - firstMapX);
            const uint32_t byteOffset =
                (static_cast<uint32_t>(mapY) * 256u + static_cast<uint16_t>(firstMapX)) * 2u;
            const uint24_t address = raw_map_data + byteOffset;
            fx_read_data_bytes(address, rowbuf + firstColumn,
                               static_cast<size_t>(cellCount) * sizeof(uint16_t));
            ++rowReads;
        }
        propertyWindow.writeRow(i, rowbuf);

        for (uint8_t j = 0; j < TilePropertyWindow::WINDOW_WIDTH; j++) {
            uint8_t screenX = (16 * j);
            uint8_t screenY = (16 * i);
            const int16_t mapX = readStartX + j;
            const uint16_t tileId = TileProps::tileId(rowbuf[j]);
            int16_t x = static_cast<int16_t>(screenX) + xOffset;
            int16_t y = static_cast<int16_t>(screenY) + yOffset;

            if (mapX < 0 || mapX >= 256 || mapY < 0 || mapY >= 256 || tileId == 0) {
                continue;
            }

            if (xShift == 1) {
                x -= 16;
            } else if (yShift == -1) {
                y -= 16;
            }
            Blit::draw(x, y, 16, 16, tiles, FRAME(tileId - 1), Blit::OVERWRITE);
        }
    }
    return rowReads;
}

#define PLAYER_SIZE 16
#define PLAYER_X_OFFSET WIDTH / 2 - PLAYER_SIZE / 2
#define PLAYER_Y_OFFSET HEIGHT / 2 - PLAYER_SIZE / 2
static void drawPlayer() {
    Blit::draw(PLAYER_X_OFFSET, PLAYER_Y_OFFSET, 16, 16, characterSheet, FRAME(0), Blit::OVERWRITE);
}
