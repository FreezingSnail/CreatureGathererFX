#include "Arena.hpp"
#include "../../player/Player.hpp"
#include "../battle/Battle.hpp"
#include "../draw.h"
#include "../menu/MenuV2.hpp"
#include <ArduboyFX.h>
#include "../../common.hpp"
#include "../../lib/ReadData.hpp"

void Arena::arenaLoop(MenuV2 &menu2, Player &player, BattleEngine &engine) {
    if (this->moveIndex == 12) {
        this->cursor = 0;
        this->movePointer = 0;
        this->moveIndex = 0;
        this->registerIndex = 0;
        this->startBattle(engine, player);
    }

    if (this->registerIndex < 3) {
        if (Arduboy2::justPressed(DOWN_BUTTON)) {
            menu2.cursorIndex += 1;
        } else if (Arduboy2::justPressed(UP_BUTTON)) {
            menu2.cursorIndex -= 1;
        }
        menu2.creatureRental();
        this->registerRentals(player, menu2);

    } else if (this->moveIndex < 12) {
        this->registerMoves(player);
    }
}

void Arena::drawarenaLoop(MenuV2 &menu2, Player &player, BattleEngine &engine) {

    if (this->registerIndex < 3) {

        menu2.creatureRental();
        this->displayRegisteredCount();
    } else if (this->moveIndex < 12) {
        this->drawregisterMoves(player);
    }
}

void Arena::registerRentals(Player &player, MenuV2 &menu2) {
    int8_t creatureID = -1;
    if (Arduboy2::justPressed(A_BUTTON)) {
        creatureID = menu2.cursorIndex;

        if (creatureID >= 0) {
            player.loadCreature(this->registerIndex, creatureID);
            this->registerIndex++;
            if (this->registerIndex == 3) {
                menu2.cursorIndex = 0;
            }
        }
    }
}

static void validMoves(uint32_t movePool, int8_t *moves) {
    uint8_t bitIndex = 0;
    uint8_t bitGood = 0;
    for (uint8_t i = 0; i < 16; i++) {
        moves[i] = -1;
    }
    for (uint8_t i = 0; i < 16; i++) {
        while (bitIndex < 32) {
            bitGood = (movePool >> (31 - bitIndex)) & 1;
            if (bitGood) {
                moves[i] = bitIndex;
                bitIndex++;
                break;
            }
            bitGood = 0;
            bitIndex++;
        }
    }
}

void Arena::registerMoves(Player &player) {
    setTextColorWhite();
    if (this->moveIndex > 7) {
        this->moveCreature = 3;
    } else if (this->moveIndex > 3) {
        this->moveCreature = 2;
    }

    // load creature move pool
    uint8_t curMonID = player.party[this->moveCreature].id;
    uint24_t addr = MoveLists::moveList + sizeof(uint32_t) * curMonID;
    uint8_t v4[4];
    FX::readDataBytes(addr, v4, sizeof(uint32_t));
    uint32_t movePool = uint32_t(v4[3]) | (uint32_t(v4[2]) << 8) | (uint32_t(v4[1]) << 16) | (uint32_t(v4[0]) << 24);
    // this->debug = movePool;
    int8_t moves[16];
    validMoves(movePool, moves);
    // font.setCursor(0, 0);
    // font.print(this->moveIndex);
    //  SpritesU::drawOverwriteFX(0, 10, CreatureNames::CreatureNames, curMonID * 3 + arduboy.currentPlane());

    for (uint8_t i = 0; i < 4; i++) {
        int8_t move = moves[this->movePointer + i];
        if (move != -1) {
            uint24_t moveAddress = FX::readIndexedUInt24(MoveData::moveNames, move);
            // printString(font, moveAddress, 10, 20 + (i * 10));
        }
    }

    if (Arduboy2::justPressed(A_BUTTON)) {
        this->cursor = moves[this->movePointer];
        player.party[this->moveCreature].setMove(this->cursor, this->moveIndex % 4);
        this->moveIndex++;
        movePointer = 0;
    }
    if (Arduboy2::justPressed(DOWN_BUTTON)) {
        if (this->movePointer < 16 && moves[this->movePointer + 1] != -1) {
            this->movePointer++;
            this->cursor = moves[this->movePointer];
        }
    }
    if (Arduboy2::justPressed(UP_BUTTON)) {
        if (this->movePointer > 0) {
            this->movePointer--;
            this->cursor = moves[this->movePointer];
        }
    }

    printMoveInfo(this->cursor, 70, 20);
}

void Arena::drawregisterMoves(Player &player) {
    setTextColorWhite();
    if (this->moveIndex > 7) {
        this->moveCreature = 3;
    } else if (this->moveIndex > 3) {
        this->moveCreature = 2;
    }

    // load creature move pool
    uint8_t curMonID = player.party[this->moveCreature].id;
    uint24_t addr = MoveLists::moveList + sizeof(uint32_t) * curMonID;
    uint8_t v4[4];
    FX::readDataBytes(addr, v4, sizeof(uint32_t));
    uint32_t movePool = uint32_t(v4[3]) | (uint32_t(v4[2]) << 8) | (uint32_t(v4[1]) << 16) | (uint32_t(v4[0]) << 24);
    // this->debug = movePool;
    int8_t moves[16];
    validMoves(movePool, moves);
    // font.setCursor(0, 0);
    // font.print(this->moveIndex);
    //  SpritesU::drawOverwriteFX(0, 10, CreatureNames::CreatureNames, curMonID * 3 + arduboy.currentPlane());
    Arduboy2::drawCircle(5, 22, 3, WHITE);

    for (uint8_t i = 0; i < 4; i++) {
        int8_t move = moves[this->movePointer + i];
        if (move != -1) {
            Serial.println(move);
            uint24_t moveAddress = FX::readIndexedUInt24(MoveNames::MoveNames, move);
            SpritesU::drawOverwriteFX(10, 20 + (i * 10), moveAddress, FRAME(0));
        }
    }

    printMoveInfo(this->cursor, 70, 20);
}

uint8_t Arena::selectOpponent() {
    return 0;
}

void Arena::startBattle(BattleEngine &engine, Player &player) {
    gameState.state = GameState_t::BATTLE;
    menu.pop();
    menu.push(MenuEnum::BATTLE_OPTIONS);
    engine.startArena(0);
}

void Arena::displayRegisteredCount() {
    for (uint8_t i = 0; i < this->registerIndex; i++) {
        Arduboy2::drawCircle(100 + (10 * i), 55, 3, WHITE);
    }
}