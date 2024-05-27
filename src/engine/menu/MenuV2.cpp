#include "MenuV2.hpp"
#include "../battle/Battle.hpp"
#include "../draw.h"
#include "../../common.hpp"

#define dbf __attribute__((optimize("-O0"))
#define CURRENT_MENU this->stack[this->menuPointer]

MenuV2::MenuV2() {
    this->menuPointer = -1;
}
// TODO: reads too much
void MenuV2::updateMoveList(BattleEngine &engine) {
    this->moveList = engine.getPlayerCurCreatureMoves();
}

void MenuV2::push(MenuEnum type) {
    this->menuPointer++;
    this->stack[this->menuPointer] = type;
}

void MenuV2::pop() {
    this->menuPointer--;
}
void MenuV2::clear() {
    this->menuPointer = -1;
    dialogMenu.dialogPointer = -1;
}

void MenuV2::transverse() {
    switch (CURRENT_MENU) {
    case BATTLE_OPTIONS:
        if (arduboy.justPressed(LEFT_BUTTON)) {
            this->cursorIndex--;
        }
        if (arduboy.justPressed(RIGHT_BUTTON)) {
            this->cursorIndex++;
        }
        if (arduboy.justPressed(DOWN_BUTTON)) {
            this->cursorIndex += 2;
        }
        if (arduboy.justPressed(UP_BUTTON)) {
            this->cursorIndex -= 2;
        }
        break;
    case BATTLE_MOVE_SELECT:
        if (arduboy.justPressed(LEFT_BUTTON)) {
            this->cursorIndex--;
        }
        if (arduboy.justPressed(RIGHT_BUTTON)) {
            this->cursorIndex++;
        }
        if (arduboy.justPressed(DOWN_BUTTON)) {
            this->cursorIndex += 2;
        }
        if (arduboy.justPressed(UP_BUTTON)) {
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
            // TODO: bug where menu doesnt pop
            engine.queueAction(ActionType::CHNGE, cursorIndex / 2);
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

void DGF MenuV2::run(BattleEngine &engine) {
    if (this->menuPointer < 0 && !dialogMenu.peek())
        return;
    if (dialogMenu.peek()) {
        if (arduboy.justPressed(A_BUTTON)) {
            dialogMenu.popMenu();
        }
    } else {
        // TODO very inefficient
        updateMoveList(engine);
        engine.updateInactiveCreatures(this->creatures);

        transverse();
        action(engine);
    }
}

void MenuV2::printMenu(BattleEngine &engine) {
    // arduboy.fillRect(0, 43, 128, 32, WHITE);
    switch (CURRENT_MENU) {
    case BATTLE_OPTIONS:
        SpritesU::drawOverwriteFX(0, 40, fightMenu, FRAME(cursorIndex));
        break;

    case BATTLE_MOVE_SELECT:
        SpritesU::drawOverwriteFX(0, 40, battleMenu, FRAME(0));
        printMoveMenu(this->cursorIndex, this->moveList);
        break;

    // TODO: Lets you pick a fainted creature
    case BATTLE_CREATURE_SELECT:
        uint8_t cIndex = this->creatures[1];
        if (this->cursorIndex == 0) {
            cIndex = this->creatures[0];
        }
        printCreatureMenu(this->creatures[0], this->creatures[1], engine.getCreature(cIndex), this->cursorIndex);
        SpritesU::drawPlusMaskFX(0, 0, ecreatureSprites, FRAME(engine.getCreature(cIndex)->id));
        break;
    }
    printCursor(this->cursorIndex);
    // arduboy.drawRect(1, 44, 126, 19, BLACK);
}

// TODO move text to drawing
void MenuV2::creatureRental() {
    // printString(font, MenuFXData::pointerText, 0, 55);
    FX::setCursor(10, 55);
    this->cursorIndex = this->cursorIndex % 31;

    SpritesU::drawOverwriteFX(0, 55, CreatureNames::CreatureNames, this->cursorIndex * 3 + arduboy.currentPlane());
    FX::drawBitmap(0, 0, creatureSprites, this->cursorIndex, dbmWhite);
    CreatureData_t cseed;
    uint24_t rowAddress = CreatureData::creatureData + (sizeof(CreatureData_t) * this->cursorIndex);
    FX::readDataObject(rowAddress, cseed);

    SpritesU::drawOverwriteFX(35, 0, hpText, FRAME(0));
    SpritesU::drawPlusMaskFX(53, 0, cseed.hpSeed, FRAME(0));

    SpritesU::drawOverwriteFX(35, 10, atkText, FRAME(0));
    SpritesU::drawPlusMaskFX(53, 10, cseed.atkSeed, FRAME(0));

    SpritesU::drawOverwriteFX(35, 20, defText, FRAME(0));
    SpritesU::drawPlusMaskFX(53, 20, cseed.defSeed, FRAME(0));

    SpritesU::drawOverwriteFX(72, 0, satkText, FRAME(0));
    SpritesU::drawPlusMaskFX(72 + 18, 0, cseed.spcAtkSeed, FRAME(0));

    SpritesU::drawOverwriteFX(72, 10, sdefText, FRAME(0));
    SpritesU::drawPlusMaskFX(72 + 18, 10, cseed.spcDefSeed, FRAME(0));

    SpritesU::drawOverwriteFX(72, 20, spdText, FRAME(0));
    SpritesU::drawPlusMaskFX(72 + 18, 20, cseed.spdSeed, FRAME(0));

    printType(Type(cseed.type1), 0, 35);
    printType(Type(cseed.type2), 0, 45);
}
