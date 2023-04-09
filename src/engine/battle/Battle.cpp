#include "Battle.hpp"
#define DEBUG

BattleEngine::BattleEngine() {
}

// battle loop
void BattleEngine::Encounter() {
    this->startEncounter();
    while(!this->checkLoss() || !this->checkWin()) {
        this->turnTick();
    }
    this->endEncounter();
}


void BattleEngine::LoadOpponent() {
}

void BattleEngine::startEncounter() {
}

void BattleEngine::turnTick() {
    this->getInput();
    this->opponentInput();
}

bool BattleEngine::checkLoss() {
    return uint8_t(this->awakeMons & 0b11100000) == uint8_t(0) ;
}

bool BattleEngine::checkWin() {
   
    return uint8_t(this->awakeMons & 0b00000111) == uint8_t(0) ;
}

void BattleEngine::endEncounter() {
}

void BattleEngine::getInput() {
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
            this->playerAction.setActionType(ActionType_t::ATTACK);
            break;
        case 5:
        case 6:
        case 7:
        default:
            break;
    }


    #endif

    // arduboy input from menu
}

void BattleEngine::opponentInput() {
    // ai to select best move
}



#ifdef DEBUG
        void BattleEngine::loadAwakeMons(uint8_t b){
            this->awakeMons = b;
        }

        void BattleEngine::printEncounter() {
        }
#endif
