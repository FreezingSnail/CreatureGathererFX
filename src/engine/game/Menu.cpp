#include "Menu.hpp"
#include "Arduboy2.h"
#include "../../external/FlashStringHelper.h"
#include "../../action/Action.hpp"
#include "../../lib/Move.hpp"
#include "../../lib/Text.hpp"


Menu::Menu(Arduboy2* arduboy) {
    this->arduboy = arduboy;
    this->cursorIndex = 0;
}

bool Menu::actionInput(Action* action) {
    if(this->queued) {
        this->queued = false;
        action->setActionType(this->queuedAction.actionType);
        action->actionIndex = this->queuedAction.actionIndex;
        return true;
    }
   return false;
}
void Menu::setState(State_t s) {
    this->state = s;
}

void Menu::printMenu() {
    this->moveCursor();
    this->printCursor();
    this->tansverseMenu();
    switch(this->state){
        case State_t::BATTLE:
            this->printBattleMenu();
        case State_t::WORLD:
            this->printWorldMenu();
        default:
            this->printBattleMenu();
    };
}

void Menu::printBattleMenu() {
    switch (this->curMenu) {
        case MenuType::BMAIN:
            this->arduboy->setCursor(6, 45);
            this->arduboy->print(asFlashStringHelper(moveM));
            this->arduboy->setCursor(64, 45);
            this->arduboy->print(asFlashStringHelper(itemM));
            this->arduboy->setCursor(6, 55);
            this->arduboy->print(asFlashStringHelper(changeM));
            this->arduboy->setCursor(64, 55);
            this->arduboy->print(asFlashStringHelper(escapeM));


            break;
        case MenuType::BMOVE:
            this->printMoveMenu();
            break;
    }
}

void Menu::printMoveMenu() {
    this->arduboy->setCursor(6, 45);
    this->arduboy->print(readFlashStringPointer(&moveNames[0]));
    this->arduboy->setCursor(64, 45);
    this->arduboy->print(readFlashStringPointer(&moveNames[1]));
    this->arduboy->setCursor(6, 55);
    this->arduboy->print(readFlashStringPointer(&moveNames[2]));
    this->arduboy->setCursor(64, 55);
    this->arduboy->print(readFlashStringPointer(&moveNames[3]));

}

void Menu::printItemMenu() {
}

void Menu::printWorldMenu() {
}

void Menu::printCreatureMenu() {
}

void Menu::printInventoryMenu() {
}


void Menu::printCursor() {
    switch (this->cursorIndex) {
        case 0:
            this->arduboy->setCursor(0, 45);
            break;
        case 1:
            this->arduboy->setCursor(58, 45);
            break;
        case 2:
            this->arduboy->setCursor(0, 55);
            break;
        case 3:
            this->arduboy->setCursor(58, 55);
            break;
    }
    this->arduboy->print(F(">"));
}

void Menu::moveCursor() {
    if (this->arduboy->justPressed(LEFT_BUTTON)) {
        this->cursorIndex--;
    } else if (this->arduboy->justPressed(RIGHT_BUTTON)) {
        this->cursorIndex++;
    }
}

void Menu::tansverseMenu() {
    if (this->arduboy->justPressed(A_BUTTON)) {
        switch(this->curMenu) {
            case BMAIN:
                switch(this->cursorIndex) {
                    case 0:
                        this->curMenu = BMOVE;
                        break;
                    case 1:
                        this->curMenu = BITEM;
                        break;
                    case 2:
                        this->curMenu = BCHANGE;
                        break;
                    case 3:
                        this->queueAction(ActionType::ESCAPE, 0);
                        // need to redeisgn how an action is qued
                        break;
                }
                this->cursorIndex = 0;
                break;

            case BMOVE:
                switch(this->cursorIndex) {
                    case 0:
                        this->queueAction(ActionType::ATTACK, 0);
                        break;
                    case 1:
                        this->queueAction(ActionType::ATTACK, 1);
                        break;
                    case 2:
                        this->queueAction(ActionType::ATTACK, 2);
                        break;
                    case 3:
                        this->queueAction(ActionType::ATTACK, 3);
                        break;
            }
            this->cursorIndex = 0;
            break;
        }
    } else if (this->arduboy->justPressed(B_BUTTON)) {
        if (this->curMenu <= BCHANGE && this->curMenu >= BMOVE) {
            this->curMenu = BMAIN;
        }
    }
 }

 void Menu::queueAction(ActionType type, uint8_t index) {
    this->queued = true;
    this->queuedAction.setActionType(type);
    this->queuedAction.actionIndex = index;
 }