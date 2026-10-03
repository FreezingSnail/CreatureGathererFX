#include "MenuV2.hpp"
#include "../battle/Battle.hpp"
#include "MenuNav.hpp"
#include "../draw.h"
#include "../../common.hpp"
#include "../../globals.hpp"

#define dbf __attribute__((optimize("-O0"))
#define CURRENT_MENU this->stack[this->menuPointer]

MenuV2::MenuV2() {
    this->menuPointer = -1;
}
// TODO: reads too much
void MenuV2::updateMoveList(BattleEngine &engine) {
    this->moveList = engine.getPlayerCurCreatureMoves();
    if (cachedNameMode != BATTLE_MOVE_SELECT) {
        cachedNameMode = BATTLE_MOVE_SELECT;
        for (uint8_t i = 0; i < 4; ++i) moveNameIds[i] = 255;
    }
    uint8_t lookups = 0;
    for (uint8_t i = 0; i < 4; ++i) {
        const uint8_t id = moveList[i];
        if (moveNameIds[i] == id) continue;
        moveNameIds[i] = id;
        moveNameAddresses[i] = id == 32 ? 0 : readMoveNameAddress(id);
        if (id != 32) ++lookups;
    }
    if (lookups != 0) FxReadCounter::transitionExact(lookups);
}

void MenuV2::updateCreatureNames(BattleEngine &engine) {
    if (cachedNameMode != BATTLE_CREATURE_SELECT) {
        cachedNameMode = BATTLE_CREATURE_SELECT;
        for (uint8_t i = 0; i < 4; ++i) moveNameIds[i] = 255;
    }
    uint8_t lookups = 0;
    for (uint8_t i = 0; i < 2; ++i) {
        const uint8_t id = creatures[i];
        if (creatureNameIds[i] == id) continue;
        creatureNameIds[i] = id;
        creatureNameAddresses[i] = readCreatureNameAddress(id);
        ++lookups;
    }
    const uint8_t selected = cursorIndex == 0 ? creatures[0] : creatures[1];
    Creature *creature = engine.getCreature(selected);
    for (uint8_t i = 0; i < 4; ++i) {
        const uint8_t id = creature->moves[i];
        if (moveNameIds[i] == id) continue;
        moveNameIds[i] = id;
        moveNameAddresses[i] = id == 32 ? 0 : readMoveNameAddress(id);
        if (id != 32) ++lookups;
    }
    if (lookups != 0) FxReadCounter::transitionExact(lookups);
}

void MenuV2::push(MenuEnum type) {
    this->menuPointer++;
    this->stack[this->menuPointer] = type;
}

void MenuV2::pop() {
    if (this->menuPointer < 0) {
        return;
    }
    this->menuPointer--;
}
void MenuV2::clear() {
    this->menuPointer = -1;
    cachedNameMode = 255;
    dialogMenu.clear();
}

void MenuV2::transverse() {
    uint8_t buttons = 0;
    if (arduboy.justPressed(LEFT_BUTTON)) buttons |= MENU_NAV_LEFT;
    if (arduboy.justPressed(RIGHT_BUTTON)) buttons |= MENU_NAV_RIGHT;
    if (arduboy.justPressed(DOWN_BUTTON)) buttons |= MENU_NAV_DOWN;
    if (arduboy.justPressed(UP_BUTTON)) buttons |= MENU_NAV_UP;

    const uint8_t cursor = this->cursorIndex < 0
                               ? 0
                               : static_cast<uint8_t>(this->cursorIndex);
    this->cursorIndex = static_cast<int8_t>(
        menuNavMove(menuDescFor(CURRENT_MENU), cursor, buttons));
}

void MenuV2::action(BattleEngine &engine) {
    if (arduboy.justPressed(A_BUTTON)) {
        switch (CURRENT_MENU) {
        case BATTLE_OPTIONS:
            switch (this->cursorIndex) {
            case 0:
                this->push(BATTLE_MOVE_SELECT);
                break;
            case 1:
                // engine.queueAction(ActionType::GATHER, 0);
                dialogMenu.pushMenu(newDialogBox(GATHERING, 0, 0));
                break;
            case 2:
                this->push(BATTLE_CREATURE_SELECT);
                break;
            case 3:
                engine.queueAction(ActionType::ESCAPE, 0);   // needredeisgn how an action is qued
                break;
            }
            this->cursorIndex = 0;
            break;

        case BATTLE_MOVE_SELECT:
            switch (this->cursorIndex) {
            case 0:
                engine.queueAction(ActionType::ATTACK, 0);
                break;
            case 1:
                engine.queueAction(ActionType::ATTACK, 1);
                break;
            case 2:
                engine.queueAction(ActionType::ATTACK, 2);
                break;
            case 3:
                engine.queueAction(ActionType::ATTACK, 3);
                break;
            }
            if (CURRENT_MENU == BATTLE_MOVE_SELECT || CURRENT_MENU == BATTLE_CREATURE_SELECT) {
                this->pop();
                this->cursorIndex = 0;
            }
            break;

        case BATTLE_CREATURE_SELECT:
            // TODO: bug where menu doesnt pop
            engine.queueAction(ActionType::CHNGE, cursorIndex / 2);
            if (CURRENT_MENU == BATTLE_MOVE_SELECT || CURRENT_MENU == BATTLE_CREATURE_SELECT) {
                this->pop();
                this->cursorIndex = 0;
            }
            break;
        case ARENA_MENU:
            return;
        }
    } else if (arduboy.justPressed(B_BUTTON)) {
        if (CURRENT_MENU == BATTLE_MOVE_SELECT || CURRENT_MENU == BATTLE_CREATURE_SELECT) {
            this->pop();
        }
    }
}

void MenuV2::run(BattleEngine &engine) {
    if (this->menuPointer < 0 && !dialogMenu.peek())
        return;
    if (dialogMenu.peek()) {
        if (arduboy.justPressed(A_BUTTON)) {
            dialogMenu.popMenu();
        }

    } else {
        // TODO very inefficient
        engine.updateInactiveCreatures(this->creatures);

        transverse();
        action(engine);
        if (menuPointer >= 0) {
            if (CURRENT_MENU == BATTLE_MOVE_SELECT) updateMoveList(engine);
            else if (CURRENT_MENU == BATTLE_CREATURE_SELECT) updateCreatureNames(engine);
        }
    }
}

void MenuV2::printMenu(BattleEngine &engine) {
    if (!this->drawMenu) {
        SpritesU::drawOverwriteFX(0, 40, 128, 24, battleMenu - 2, FRAME(0));
        return;
    }
    if (this->menuPointer < 0) {
        SpritesU::drawOverwriteFX(0, 40, 128, 24, battleMenu - 2, FRAME(0));
        return;
    }
    // arduboy.fillRect(0, 43, 128, 32, WHITE);
    switch (CURRENT_MENU) {
    case BATTLE_OPTIONS:
        SpritesU::drawOverwriteFX(0, 40, 128, 24, fightMenu - 2, FRAME(cursorIndex));
        break;

    case BATTLE_MOVE_SELECT:
        SpritesU::drawOverwriteFX(0, 40, 128, 24, battleMenu - 2, FRAME(0));
        printMoveMenu(this->cursorIndex, this->moveList,
                      moveNameAddresses, engine.playerCur->moveList[cursorIndex]);
        break;

    // TODO: Lets you pick a fainted creature
    case BATTLE_CREATURE_SELECT:
        uint8_t cIndex = this->creatures[1];
        if (this->cursorIndex == 0) {
            cIndex = this->creatures[0];
        }
        printCreatureMenu(this->creatures[0], this->creatures[1], engine.getCreature(cIndex),
                          this->cursorIndex, creatureNameAddresses, moveNameAddresses);
        SpritesU::drawPlusMaskFX(0, 0, 32, 32, NewecreatureSprites - 2, FRAME((engine.getCreature(cIndex)->id * 2)));
        break;
    }
    printCursor(this->cursorIndex);
    // arduboy.drawRect(1, 44, 126, 19, BLACK);
}

// TODO move textdrawing
void MenuV2::prepareCreatureRental() {
    if (cursorIndex > 30) {
        cursorIndex = 0;
    } else if (cursorIndex < 0) {
        cursorIndex = 30;
    }
    if (cachedRentalId == static_cast<uint8_t>(cursorIndex)) return;
    cachedRentalId = static_cast<uint8_t>(cursorIndex);
    rentalNameAddress = readCreatureNameAddress(cachedRentalId);
    const CreatureData_t seed = getCreatureFromStore(cachedRentalId);
    rentalSeed = {seed.type1, seed.type2, seed.hpSeed, seed.atkSeed,
                  seed.defSeed, seed.spcAtkSeed, seed.spcDefSeed, seed.spdSeed};
    FxReadCounter::transitionExact(2);
}

void MenuV2::creatureRental() {
    // printString(font, MenuFXData::pointerText, 0, 55);
    FX::setCursor(10, 55);
    if (cachedRentalId != static_cast<uint8_t>(cursorIndex)) return;
    drawStringSprite(0, 55, rentalNameAddress, readCreatureNameWidth(cachedRentalId), FRAME(0));
    SpritesU::drawPlusMaskFX(0, 0, 32, 32, NewecreatureSprites - 2, FRAME((this->cursorIndex * 2)));

    // FX::drawBitmap(0, 0, NewecreatureSprites, FRAME((this->cursorIndex * 2)), dbmWhite);
    const RentalStats &cseed = rentalSeed;

    drawStringSprite(35, 0, hpText, 20, FRAME(0));
    drawStatNumbers(60, 0, cseed.hpSeed);

    drawStringSprite(35, 10, atkText, 25, FRAME(0));
    drawStatNumbers(60, 10, cseed.atkSeed);

    drawStringSprite(35, 20, defText, 25, FRAME(0));
    drawStatNumbers(60, 20, cseed.defSeed);

    drawStringSprite(72, 0, satkText, 30, FRAME(0));
    drawStatNumbers(103, 0, cseed.spcAtkSeed);

    drawStringSprite(72, 10, sdefText, 30, FRAME(0));
    drawStatNumbers(103, 10, cseed.spcDefSeed);

    drawStringSprite(72, 20, spdText, 25, FRAME(0));
    drawStatNumbers(103, 20, cseed.spdSeed);

    printType(Type(cseed.type1), 0, 35);
    printType(Type(cseed.type2), 0, 45);
}
