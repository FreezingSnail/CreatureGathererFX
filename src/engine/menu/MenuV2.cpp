#include "MenuV2.hpp"
#include "../battle/Battle.hpp"
#include "../draw.h"
#include "../../common.hpp"
#include "../../globals.hpp"

#define dbf __attribute__((optimize("-O0"))
#define CURRENT_MENU this->stack[this->menuPointer]

void updateFightState() {
    switch (engine.turnState) {
    case BattleState::TURN_INPUT: {
        // handled automatically
    } break;
    case BattleState::PLAYER_ATTACK:
        engine.turnState = BattleState::OPPONENT_RECEIVE_DAMAGE;
        break;
    case BattleState::OPPONENT_RECEIVE_DAMAGE:
        engine.turnState = BattleState::OPPONENT_RECEIVE_EFFECT_APPLICATION;
        break;
    case BattleState::OPPONENT_ATTACK:
        engine.turnState = BattleState::PLAYER_RECEIVE_DAMAGE;
        break;
    case BattleState::PLAYER_RECEIVE_DAMAGE:
        engine.turnState = BattleState::PLAYER_RECEIVE_EFFECT_APPLICATION;
        break;
    case BattleState::PLAYER_RECEIVE_EFFECT_APPLICATION:
        if (engine.PlayerActionReady()) {
            engine.turnState = BattleState::PLAYER_ATTACK;
        } else {
            engine.turnState = BattleState::END_TURN;
        }
        break;
    case BattleState::OPPONENT_RECEIVE_EFFECT_APPLICATION:
        if (engine.OpponentActionReady()) {
            engine.turnState = BattleState::OPPONENT_ATTACK;
        } else {
            engine.turnState = BattleState::END_TURN;
        }
        break;
    case BattleState::END_TURN:
        engine.turnState = BattleState::TURN_INPUT;
        break;
    }
}

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
    case ARENA_MENU:
        if (arduboy.justPressed(LEFT_BUTTON)) {
            this->cursorIndex--;
        } else if (arduboy.justPressed(RIGHT_BUTTON)) {
            this->cursorIndex++;
        }
        return;
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

    if (engine.updateState) {
        updateFightState();
        engine.updateState = false;
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
        SpritesU::drawPlusMaskFX(0, 0, NewecreatureSprites, FRAME((engine.getCreature(cIndex)->id * 2)));
        break;
    }
    printCursor(this->cursorIndex);
    // arduboy.drawRect(1, 44, 126, 19, BLACK);
}

// TODO move textdrawing
void MenuV2::creatureRental() {
    // printString(font, MenuFXData::pointerText, 0, 55);
    FX::setCursor(10, 55);
    if (cursorIndex > 30) {
        cursorIndex = 0;
    } else if (cursorIndex < 0) {
        cursorIndex = 30;
    }
    uint24_t addr = FX::readIndexedUInt24(CreatureNames::CreatureNames, this->cursorIndex);
    SpritesU::drawOverwriteFX(0, 55, addr, FRAME(0));
    SpritesU::drawPlusMaskFX(0, 0, NewecreatureSprites, FRAME((this->cursorIndex * 2)));

    // FX::drawBitmap(0, 0, NewecreatureSprites, FRAME((this->cursorIndex * 2)), dbmWhite);
    CreatureData_t cseed;
    uint24_t rowAddress = CreatureData::creatureData + (sizeof(CreatureData_t) * this->cursorIndex);
    FX::readDataObject(rowAddress, cseed);

    SpritesU::drawOverwriteFX(35, 0, hpText, FRAME(0));
    drawStatNumbers(60, 0, cseed.hpSeed);

    SpritesU::drawOverwriteFX(35, 10, atkText, FRAME(0));
    drawStatNumbers(60, 10, cseed.atkSeed);

    SpritesU::drawOverwriteFX(35, 20, defText, FRAME(0));
    drawStatNumbers(60, 20, cseed.defSeed);

    SpritesU::drawOverwriteFX(72, 0, satkText, FRAME(0));
    drawStatNumbers(103, 0, cseed.spcAtkSeed);

    SpritesU::drawOverwriteFX(72, 10, sdefText, FRAME(0));
    drawStatNumbers(103, 10, cseed.spcDefSeed);

    SpritesU::drawOverwriteFX(72, 20, spdText, FRAME(0));
    drawStatNumbers(103, 20, cseed.spdSeed);

    printType(Type(cseed.type1), 0, 35);
    printType(Type(cseed.type2), 0, 45);
}
