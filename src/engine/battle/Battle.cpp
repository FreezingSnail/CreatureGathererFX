#include "Battle.hpp"
#include "creature/Creature.hpp"
#include "opponent/Opponent.hpp"
#include "player/Player.hpp"
#include "lib/TypeTable.hpp"
#include "lib/Move.hpp"
#ifdef CLI
#include <iostream>
#endif

uint16_t calculateDamage(Action* action, Creature* committer, Creature* reciever) {
    //need to do something here with atk def stats
    uint8_t move = moveList[committer->moves[action->actionIndex]];
    float mod = getMatchupModifier(getMoveType(move), uint8_t(reciever->type))/2;
    uint8_t power = getMovePower(move);
    #ifdef CLI
        std::cout << "power: " << (unsigned)power << " attack stat: " << (unsigned)committer->getAtkStat() << " defense stat: " << (unsigned)reciever->getDefStat() << std::endl;
        std::cout << "move type: " << (unsigned)getMoveType(move) << " reciever type: " << (unsigned)reciever->type << " modifier: " << std::fixed << mod << std::endl;
    #endif
    uint16_t damage = ((power * committer->getAtkStat() / reciever->getDefStat()) * mod);
    if ( damage == 0 ){
        if (mod == 0) {
            return 0;
        }
        return 1;
    }

    // going too need to balance this eventually
    return damage;
}

BattleEngine::BattleEngine() {
}

// battle loop
void BattleEngine::encounter(Player* player, OpponentSeed* seed) {
    this->startEncounter(player, seed);
    while(true) {
        if (this->checkLoss()) {
            #ifdef CLI
                std::cout << "You have been defeated" << std::endl;
            #endif
            break;
        }
        
        if(this->checkWin()) {
            #ifdef CLI
                std::cout << "You have won the battle" << std::endl;
            #endif
            break;
        }
        #ifdef CLI
            this->printEncounter();
            this->playerCur->printMoves();
        #endif
        this->turnTick();
    }
    this->endEncounter();
}


void BattleEngine::loadOpponent(OpponentSeed* seed) {
    this->opponent.load(seed);
    this->opponentIndex = 0;
    this->opponentCur = &(this->opponent.party[0]);
    this->opponentHealths[0] = this->opponent.party[0].getHpStat();
    this->opponentHealths[1] = this->opponent.party[1].getHpStat();
    this->opponentHealths[2] = this->opponent.party[2].getHpStat();
}

void BattleEngine::loadPlayer(Player* player) {
    this->playerParty[0] = &(player->party[0]);
    this->playerParty[1] = &(player->party[1]);
    this->playerParty[2] = &(player->party[2]);
    // for now the hp will refil every encounter so we dont need to use the player field
    //this->playerHealths[i] = player->creatureHPs[i];
    this->playerHealths[0] = this->playerParty[0]->getHpStat();
    this->playerHealths[1] = this->playerParty[1]->getHpStat();
    this->playerHealths[2] = this->playerParty[2]->getHpStat();
    
    this->playerIndex = 0;
    this->playerCur = this->playerParty[0];
}

void BattleEngine::startEncounter(Player* player, OpponentSeed* seed) {
    this->loadOpponent(seed);
    this->loadPlayer(player);
}

//this function is terrible
void BattleEngine::turnTick() {
    this->getInput();
    this->opponentInput();
    int8_t order = this->playerAction.priority != this->opponentAction.priority;
    if(order > 0){
        this->playerActionFirst();
    } else if (order < 0) {
        this->opponentActionFirst();
    } else {
        order = this->playerCur->getSpdStat() - this->opponentCur->getSpdStat();
        if(order > 0 || order == 0){
            this->playerActionFirst();
        } else if (order < 0) {
            this->opponentActionFirst();
        }
    }
}

void BattleEngine::playerActionFirst() {
    #ifdef CLI
        std::cout << "player action \n";
    #endif
    this->commitAction(&this->playerAction, this->playerCur, this->opponentCur);
    this->checkOpponentFaint();
    #ifdef CLI
        std::cout << "opponent action \n";
    #endif
    this->commitAction(&this->opponentAction, this->opponentCur, this->playerCur);
    this->checkPlayerFaint();
}

void BattleEngine::opponentActionFirst() {
    #ifdef CLI
        std::cout << "opponent action \n";
    #endif
    this->commitAction(&this->opponentAction, this->opponentCur, this->playerCur);
    this->checkPlayerFaint();
    #ifdef CLI
        std::cout << "player action \n";
    #endif
    this->commitAction(&this->playerAction, this->playerCur, this->opponentCur);
    this->checkOpponentFaint();

}

void BattleEngine::checkPlayerFaint() {
    if (this->playerHealths[this->playerIndex] <= 0 ){
        #ifdef CLI
        std::cout << "player faint \n";
        #endif
        this->playerIndex++;
        this->playerCur = this->playerParty[this->playerIndex];
        //this->awakeMons &= ~(1 << this->playerIndex);
    }
}

void  BattleEngine::checkOpponentFaint() {
    if (this->opponentHealths[this->opponentIndex] <= 0 ){
        #ifdef CLI
        std::cout << "opponent faint \n";
        #endif
        this->opponentIndex++;
        this->opponentCur = &(this->opponent.party[this->opponentIndex]);
        //this->awakeMons &= ~(1 << this->opponentIndex+5);
    }
}

bool BattleEngine::checkLoss() {
    //return uint8_t(this->awakeMons & 0b11100000) == uint8_t(0) ;
    return (this->playerHealths[0] <= 0 && this->playerHealths[1] <= 0 && this->playerHealths[2] <= 0);
}

bool BattleEngine::checkWin() {
    //return uint8_t(this->awakeMons & 0b00000111) == uint8_t(0) ;
    return (this->opponentHealths[0] <= 0 && this->opponentHealths[1] <= 0 && this->opponentHealths[2] <= 0);
}

void BattleEngine::endEncounter() {

    #ifdef CLI
        std::cout << "end encounter \n";
    #endif
}

void BattleEngine::getInput() {
    // arduboy input from menu
    this->menu->actionInput(&this->playerAction);
}

void BattleEngine::opponentInput() {
    // ai to select best move

    //For now just do the first slot attack
    this->opponentAction.setActionType(ActionType_t::ATTACK);
    this->opponentAction.actionIndex = 1;
}



#ifdef CLI
        void BattleEngine::loadAwakeMons(uint8_t b){
            this->awakeMons = b;
        }

        void BattleEngine::printEncounter() {
            std::cout << "PLAYER: cur: " << (unsigned)this->playerIndex << " lvl: " << (unsigned)this->playerCur->level;
            std::cout << " type: " << (unsigned)this->playerCur->type << "\nhp:";
            for(int i = 0; i < 3; i++ ){ std::cout << " " << (unsigned)this->playerHealths[i];}
            std::cout << std::endl;
            std::cout << "Opponent: cur:" << (unsigned)this->opponentIndex << " lvl: " << (unsigned)this->opponentCur->level;
            std::cout << " type: " << (unsigned)this->opponentCur->type  << "\nhp:";
            for(int i = 0; i < 3; i++ ){ std::cout << " " << (unsigned)this->opponentHealths[i];} 
            std::cout << std::endl;
        }
#endif

void BattleEngine::commitAction(Action* action, Creature* commiter, Creature* reciever) {
    switch (action->actionType) {
    case ActionType_t::ATTACK: {
        uint16_t damage = calculateDamage(action, commiter, reciever);
        #ifdef CLI
            std::cout << "Attacking with index " << (unsigned)action->actionIndex << " Damage: " << unsigned(damage) << std::endl;
        #endif
        applyDamage(damage, reciever);
        break;
    }
    case ActionType_t::ITEM:
        break;
    case ActionType_t::CHANGE:
        break;
    case ActionType_t::ESCAPE:
        // should add a check in here for opponent vs random encounter
        this->endEncounter();
        break;
    
    default:
        break;
    }
}


void BattleEngine::applyDamage(uint16_t damage, Creature* reciever) {
    if( reciever == this->playerCur) {
        uint8_t hp = this->playerHealths[this->playerIndex];
        this->playerHealths[this->playerIndex] -= damage >= hp ? hp : damage;
    } else {
        uint8_t hp = this->opponentHealths[this->opponentIndex];
        this->opponentHealths[this->opponentIndex] -= damage >= hp ? hp : damage;

    }

}