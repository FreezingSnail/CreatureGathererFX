#include "Arena.hpp"
#include "../../player/Player.hpp"
#include "../battle/BattleSession.hpp"
#include "../draw.h"
#include "../menu/MenuV2.hpp"
#include <ArduboyFX.h>
#include "../../common.hpp"
#include "../../lib/ReadData.hpp"

bool Arena::arenaLoop(MenuV2 &menu2, Player &player) {
    // This seems wrong
    if (this->moveIndex == 11) {
        this->cursor = 0;
        this->movePointer = 0;
        this->moveIndex = 0;
        this->registerIndex = 0;
        return true;
    }
    if (this->registerIndex < 3) {
        if (Arduboy2::justPressed(DOWN_BUTTON)) {
            menu2.cursorIndex += 1;
        } else if (Arduboy2::justPressed(UP_BUTTON)) {
            menu2.cursorIndex -= 1;
        }
        menu2.prepareCreatureRental();
        this->registerRentals(player, menu2);
        if (this->registerIndex < 3) menu2.prepareCreatureRental();
    } else if (this->moveIndex < 12) {
        this->registerMoves(player);
    }
    return false;
}

void DGF Arena::drawarenaLoop(MenuV2 &menu2, Player &player) {

    if (this->registerIndex < 3) {
        menu2.creatureRental();
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
            FxReadCounter::transitionExact(5);
            this->registerIndex++;
            if (this->registerIndex == 3) {
                menu2.cursorIndex = 0;
            }
        }
    }
}

static int8_t moveAt(uint32_t movePool, uint8_t slot) {
    for (uint8_t bit = 0; bit < 32; ++bit) {
        if ((movePool & (uint32_t(1) << (31 - bit))) == 0) continue;
        if (slot == 0) return bit;
        --slot;
    }
    return -1;
}

void Arena::prepareMovePool(Player &player) {
    const uint8_t curMonID = player.party[moveCreature].id;
    if (cachedPoolCreatureId == curMonID) return;
    cachedPoolCreatureId = curMonID;
    cachedMovePointer = 255;
    cachedInfoMove = 255;
    const uint24_t addr = MoveLists::moveList + sizeof(uint32_t) * curMonID;
    uint8_t v4[4];
    FxRead::bytes(addr, v4, sizeof(uint32_t));
    FxReadCounter::transitionExact(1);
    movePool = uint32_t(v4[3]) | (uint32_t(v4[2]) << 8) |
               (uint32_t(v4[1]) << 16) | (uint32_t(v4[0]) << 24);
}

void Arena::prepareVisibleMoves() {
    uint8_t lookups = 0;
    if (cachedMovePointer != movePointer) {
        cachedMovePointer = movePointer;
        for (uint8_t i = 0; i < 4; ++i) {
            const uint8_t index = movePointer + i;
            const int8_t move = index < 16 ? moveAt(movePool, index) : -1;
            visibleMoveAddresses[i] = move < 0 ? 0 : readMoveNameAddress(move);
            if (move >= 0) ++lookups;
        }
    }
    const int8_t move = moveAt(movePool, movePointer);
    if (move >= 0 && cachedInfoMove != static_cast<uint8_t>(move)) {
        cachedInfoMove = static_cast<uint8_t>(move);
        selectedMove = readMoveFX(cachedInfoMove);
        ++lookups;
    } else if (move < 0) {
        cachedInfoMove = 255;
    }
    if (lookups != 0) FxReadCounter::transitionExact(lookups);
}

void Arena::drawregisterMoves(Player &player) {
    for (uint8_t i = 0; i < 4; i++) {
        const uint8_t index = movePointer + i;
        const int8_t move = index < 16 ? moveAt(movePool, index) : -1;
        if (move != -1) {
            drawStringSprite(10, 20 + (i * 10), visibleMoveAddresses[i],
                             readMoveNameWidth(move), FRAME(0));
        }
    }
    if (cachedInfoMove != 255) printMoveInfo(cachedInfoMove, 70, 20, selectedMove);
}

void Arena::registerMoves(Player &player) {
    if (this->moveIndex > 7) {
        this->moveCreature = 3;
    } else if (this->moveIndex > 3) {
        this->moveCreature = 2;
    }

    prepareMovePool(player);

    if (Arduboy2::justPressed(A_BUTTON) && moveAt(movePool, movePointer) >= 0) {
        this->cursor = moveAt(movePool, this->movePointer);
        player.party[this->moveCreature].setMove(this->cursor, this->moveIndex % 4);
        FxReadCounter::transitionExact(1);
        this->moveIndex++;
        movePointer = 0;
    }
    if (Arduboy2::justPressed(DOWN_BUTTON)) {
        if (this->movePointer < 15 && moveAt(movePool, this->movePointer + 1) != -1) {
            this->movePointer++;
            this->cursor = moveAt(movePool, this->movePointer);
        }
    }
    if (Arduboy2::justPressed(UP_BUTTON)) {
        if (this->movePointer > 0) {
            this->movePointer--;
            this->cursor = moveAt(movePool, this->movePointer);
        }
    }

    prepareVisibleMoves();
}

uint8_t Arena::selectOpponent() {
    return 0;
}

void Arena::startBattle(battle::BattleSession &session, Player &player, MenuV2 &menu2) {
    (void)player;
    (void)menu2;
    session.beginTrainer(4);
}

void Arena::displayRegisteredCount() {
    for (uint8_t i = 0; i < this->registerIndex; i++) {
        Arduboy2::drawCircle(100 + (10 * i), 55, 3, WHITE);
    }
}
