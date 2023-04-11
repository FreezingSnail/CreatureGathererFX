#include "Menu.hpp"
#include "action/Action.hpp"
#ifdef DEBUG

Menu::Menu() {
}

void Menu::actionInput(Action* action) {
    Action playerAction
    #ifdef DEBUG
    //cli interface
    std::cout << "1-4 attacks. 5 item, 6 switch, 7 run" << std::endl;
    std::string input;
    std::cin >> input;

    switch(std::stoi(input)){
        case 1:
        case 2:
        case 3:
        case 4:
            action setActionType(ActionType_t::ATTACK);
            break;
        case 5:
            action setActionType(ActionType_t::ITEM);
            break;
        case 6:
            action setActionType(ActionType_t::SWITCH;
            break;
        case 7:
            action setActionType(ActionType_t::ESCAPE);
            break;
        default:
            break;
    }


    #endif
    Action returnValue;
    return returnValue;
}
