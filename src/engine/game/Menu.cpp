#include "Menu.hpp"
#include "../../action/Action.hpp"

#ifdef CLI
    #include <iostream>
#endif

Menu::Menu() {
}

void Menu::actionInput(Action* action) {

    #ifdef CLI
    //cli interface
    std::cout << "0-3 attacks. 5 item, 6 switch, 7 run" << std::endl;
    std::string input = "";
    std::cin >> input;
    int in = std::stoi(input);
    action->actionIndex = in;

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

    #endif
}
