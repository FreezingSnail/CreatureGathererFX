#include "Menu.hpp"
#include "Arduboy2.h"
#include "../../external/FlashStringHelper.h"
#include "../../action/Action.hpp"
#include "../../lib/Move.hpp"

Menu::Menu(Arduboy2* arduboy) {
    this->arduboy = arduboy;
}

void Menu::actionInput(Action* action) {
    action->actionIndex = 0;
/*
    switch(in){
        case 0:
        case 1:
        case 2:
        case 3:
            action->setActionType(ActionType::ATTACK);
            break;
        case 5:
            action->setActionType(ActionType::ITEM);
            break;
        case 6:
            action->setActionType(ActionType::CHNGE);
            break;
        case 7:
            action->setActionType(ActionType::ESCAPE);
            break;
        default:
            break;
    }
    */
}
void Menu::setState(State_t s) {
    this->state = s;
}

void Menu::printCursor() {
}

void Menu::printMenu() {
    switch(this->state){
        case State_t::BATTLE:
            this->printBattleMenu();
        case State_t::WORLD:
            this->printWorldMenu();
    };
}

void Menu::printBattleMenu() {

}

void Menu::printMoveMenu() {
    uint8_t moveIndex = 0;
    this->arduboy->print(readFlashStringPointer(&moveNames[moveIndex]));

}

void Menu::printItemMenu() {
}

void Menu::printWorldMenu() {
}

void Menu::printCreatureMenu() {
}

void Menu::printInventoryMenu() {
}
