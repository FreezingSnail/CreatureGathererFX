#include "MenuV2.hpp"
#include "../../external/FlashStringHelper.h"
#include "../../lib/Text.hpp"
#include "../battle/Battle.hpp"
#include "../draw.h"

#define dbf __attribute__((optimize("-O0"))
#define CURRENT_MENU this->stack[this->menuPointer]

MenuV2::MenuV2() { this->menuPointer = -1; }
void MenuV2::updateMoveList(BattleEngine &engine) { this->moveList = engine.getPlayerCurCreatureMoves(); }

void MenuV2::push(MenuEnum type) {
    this->menuPointer++;
    this->stack[this->menuPointer] = type;
}

void MenuV2::pop() { this->menuPointer--; }

void MenuV2::transverse(Arduboy2Base &arduboy) {
    switch (CURRENT_MENU) {
    case BATTLE_OPTIONS:
        if (arduboy.justPressed(LEFT_BUTTON)) {
            this->cursorIndex--;
        } else if (arduboy.justPressed(RIGHT_BUTTON)) {
            this->cursorIndex++;
        }
        if (arduboy.justPressed(DOWN_BUTTON)) {
            this->cursorIndex += 2;
        } else if (arduboy.justPressed(UP_BUTTON)) {
            this->cursorIndex -= 2;
        }
        break;
    case BATTLE_MOVE_SELECT:
        if (arduboy.justPressed(LEFT_BUTTON)) {
            this->cursorIndex--;
        } else if (arduboy.justPressed(RIGHT_BUTTON)) {
            this->cursorIndex++;
        }
        if (arduboy.justPressed(DOWN_BUTTON)) {
            this->cursorIndex += 2;
        } else if (arduboy.justPressed(UP_BUTTON)) {
            this->cursorIndex -= 2;
        }
        break;
    case BATTLE_CREATURE_SELECT:
        if (arduboy.justPressed(DOWN_BUTTON)) {
            this->cursorIndex += 2;
        } else if (arduboy.justPressed(UP_BUTTON)) {
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

void MenuV2::action(Arduboy2Base &arduboy, BattleEngine &engine) {
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
    } else if (arduboy.justPressed(B_BUTTON)) {
        if (CURRENT_MENU == BATTLE_MOVE_SELECT || CURRENT_MENU == BATTLE_CREATURE_SELECT) {
            this->pop();
        }
    }
}

void MenuV2::run(Arduboy2Base &arduboy, BattleEngine &engine) {
    if (this->menuPointer < 0 && !dialogMenu.peek())
        return;
    if (dialogMenu.peek()) {
        dialogMenu.drawPopMenu();
        if (arduboy.justPressed(A_BUTTON)) {
            dialogMenu.popMenu();
        }
    } else {
        this->updateMoveList(engine);
        engine.updateInactiveCreatures(this->creatures);
        this->transverse(arduboy);
        this->action(arduboy, engine);
        this->printMenu(arduboy, engine);
    }
}

void MenuV2::printMenu(Arduboy2Base &arduboy, BattleEngine &engine) {
    arduboy.fillRect(0, 43, 128, 32, WHITE);
    setTextColorBlack();
    switch (CURRENT_MENU) {
    case BATTLE_OPTIONS:
        printBattleMenu();
        break;

    case BATTLE_MOVE_SELECT:
        printMoveMenu(arduboy, this->cursorIndex, this->moveList);
        break;

    case BATTLE_CREATURE_SELECT:
        uint8_t cIndex = this->creatures[1];
        if (this->cursorIndex == 0) {
            cIndex = this->creatures[0];
        }
        printCreatureMenu(arduboy, this->creatures[0], this->creatures[1], engine.getCreature(cIndex));
        break;
    }
    printCursor(arduboy, this->cursorIndex);
    arduboy.drawRect(1, 44, 126, 19, BLACK);
}

void MenuV2::creatureRental(Arduboy2Base &arduboy) {
    FX::setCursor(0, 55);
    FX::drawString(MenuFXData::pointerText);
    FX::setCursor(10, 55);
    this->cursorIndex = this->cursorIndex % 31;

    uint24_t addr = FX::readIndexedUInt24(CreatureData::creatureNames, this->cursorIndex);
    FX::drawString(addr);
    FX::drawBitmap(0, 0, creatureSprites, this->cursorIndex, dbmWhite);
    CreatureData_t cseed;
    uint24_t rowAddress = CreatureData::creatureData + (sizeof(CreatureData_t) * this->cursorIndex);
    FX::readDataObject(rowAddress, cseed);
    FX::setCursor(35, 0);
    FX::drawString(MenuFXData::hpText);
    FX::drawNumber(cseed.hpSeed);
    FX::setCursor(35, 10);
    FX::drawString(MenuFXData::atkText);
    FX::drawNumber(cseed.atkSeed);
    FX::setCursor(35, 20);
    FX::drawString(MenuFXData::defText);
    FX::drawNumber(cseed.defSeed);
    FX::setCursor(72, 0);
    FX::drawString(MenuFXData::satkText);
    FX::drawNumber(cseed.spcAtkSeed);
    FX::setCursor(72, 10);
    FX::drawString(MenuFXData::sdefText);
    FX::drawNumber(cseed.spcDefSeed);
    FX::setCursor(72, 20);
    FX::drawString(MenuFXData::spdText);
    FX::drawNumber(cseed.spdSeed);
    FX::setCursor(0, 35);
    printType(Type(cseed.type1));
    FX::setCursor(0, 45);
    printType(Type(cseed.type2));
}