#include "MenuV2.hpp"
#include "../../external/FlashStringHelper.h"
#include "../battle/Battle.hpp"
#include "../draw.h"
#include "../../lib/Text.hpp"
#include "../../Globals.hpp"

#define dbf __attribute__((optimize("-O0"))
#define CURRENT_MENU this->stack[this->menuPointer]

MenuV2::MenuV2() { this->menuPointer = -1; }
void MenuV2::updateMoveList(BattleEngine &engine) { this->moveList = engine.getPlayerCurCreatureMoves(); }

void MenuV2::push(MenuEnum type) {
    this->menuPointer++;
    this->stack[this->menuPointer] = type;
}

void MenuV2::pop() { this->menuPointer--; }

void MenuV2::transverse() {
    switch (CURRENT_MENU) {
    case BATTLE_OPTIONS:
        if (Arduboy2::justPressed(LEFT_BUTTON)) {
            this->cursorIndex--;
        } else if (Arduboy2::justPressed(RIGHT_BUTTON)) {
            this->cursorIndex++;
        }
        if (Arduboy2::justPressed(DOWN_BUTTON)) {
            this->cursorIndex += 2;
        } else if (Arduboy2::justPressed(UP_BUTTON)) {
            this->cursorIndex -= 2;
        }
        break;
    case BATTLE_MOVE_SELECT:
        if (Arduboy2::justPressed(LEFT_BUTTON)) {
            this->cursorIndex--;
        } else if (Arduboy2::justPressed(RIGHT_BUTTON)) {
            this->cursorIndex++;
        }
        if (Arduboy2::justPressed(DOWN_BUTTON)) {
            this->cursorIndex += 2;
        } else if (Arduboy2::justPressed(UP_BUTTON)) {
            this->cursorIndex -= 2;
        }
        break;
    case BATTLE_CREATURE_SELECT:
        if (Arduboy2::justPressed(DOWN_BUTTON)) {
            this->cursorIndex += 2;
        } else if (Arduboy2::justPressed(UP_BUTTON)) {
            this->cursorIndex -= 2;
        }
        break;
    }
    if (this->cursorIndex > 3) {
        this->cursorIndex = 0;
    }
    if (this->cursorIndex < 0) {
        this->cursorIndex = 3;
    }
}

void MenuV2::action(BattleEngine &engine) {
    if (Arduboy2::justPressed(A_BUTTON)) {
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
                engine.queueAction(ActionType::ESCAPE, 0);   // need to redeisgn how an action is qued
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
            switch (this->cursorIndex) {
            case 0:
                engine.queueAction(ActionType::CHNGE, this->creatures[0]);
                break;
            case 2:
                engine.queueAction(ActionType::CHNGE, this->creatures[1]);
                break;
            }
            if (CURRENT_MENU == BATTLE_MOVE_SELECT || CURRENT_MENU == BATTLE_CREATURE_SELECT) {
                this->pop();
                this->cursorIndex = 0;
            }
            break;
        }
    } else if (Arduboy2::justPressed(B_BUTTON)) {
        if (CURRENT_MENU == BATTLE_MOVE_SELECT || CURRENT_MENU == BATTLE_CREATURE_SELECT) {
            this->pop();
        }
    }
}

void MenuV2::run(BattleEngine &engine) {
    if (this->menuPointer < 0 && !dialogMenu.peek())
        return;
    if (dialogMenu.peek()) {
        dialogMenu.drawPopMenu();
        if (Arduboy2::justPressed(A_BUTTON)) {
            dialogMenu.popMenu();
        }
    } else {
        this->updateMoveList(engine);
        engine.updateInactiveCreatures(this->creatures);
        this->transverse();
        this->action(engine);
        this->printMenu(engine);
    }
}

void MenuV2::printMenu(BattleEngine &engine) {
    // Arduboy2::fillRect(0, 43, 128, 32, WHITE);
    FX::drawBitmap(0, 43, battleMenu, 0, dbmNormal);
    setTextColorBlack();
    switch (CURRENT_MENU) {
    case BATTLE_OPTIONS:
        printBattleMenu();
        break;

    case BATTLE_MOVE_SELECT:
        printMoveMenu(this->cursorIndex, this->moveList);
        break;

    case BATTLE_CREATURE_SELECT:
        uint8_t cIndex = this->creatures[1];
        if (this->cursorIndex == 0) {
            cIndex = this->creatures[0];
        }
        printCreatureMenu(this->creatures[0], this->creatures[1], engine.getCreature(cIndex));
        break;
    }
    printCursor(this->cursorIndex);
    // Arduboy2::drawRect(1, 44, 126, 19, BLACK);
}

void MenuV2::creatureRental() {
    printString(font, MenuFXData::pointerText, 0, 55);
    FX::setCursor(10, 55);
    this->cursorIndex = this->cursorIndex % 31;

    uint24_t addr = FX::readIndexedUInt24(CreatureData::creatureNames, this->cursorIndex);
    printString(font, addr, 10, 55);
    FX::drawBitmap(0, 0, creatureSprites, this->cursorIndex, dbmWhite);
    CreatureData_t cseed;
    uint24_t rowAddress = CreatureData::creatureData + (sizeof(CreatureData_t) * this->cursorIndex);
    FX::readDataObject(rowAddress, cseed);
    printString(font, MenuFXData::hpText, 35, 0);
    font.setCursor(35 + 18, 0);
    font.print(cseed.hpSeed);
    printString(font, MenuFXData::atkText, 35, 10);
    font.setCursor(35 + 18, 10);
    font.print(cseed.atkSeed);
    printString(font, MenuFXData::defText, 35, 20);
    font.setCursor(35 + 18, 20);
    font.print(cseed.defSeed);
    printString(font, MenuFXData::satkText, 72, 0);
    font.setCursor(72 + 23, 0);
    font.print(cseed.spcAtkSeed);
    printString(font, MenuFXData::sdefText, 72, 10);
    font.setCursor(72 + 23, 10);
    font.print(cseed.spcDefSeed);
    printString(font, MenuFXData::spdText, 72, 20);
    font.setCursor(72 + 23, 20);
    font.print(cseed.spdSeed);
    printType(Type(cseed.type1), 0, 35);
    printType(Type(cseed.type2), 0, 45);
}