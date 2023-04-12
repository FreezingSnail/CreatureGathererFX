#include "Battle.hpp"
#include "creature/Creature.hpp"
#include "opponent/Opponent.hpp"
#include "lib/TypeTable.hpp"
#include "lib/Move.hpp"

uint16_t calculateDamage(Action* action, Creature* committer, Creature* reciever) {
    //need to do something here with atk def stats
    uint8_t move = committer->moves[action->actionIndex];
    uint8_t mod = getMatchupModifier(getMoveType(move), uint8_t(reciever->type));
    uint8_t power = getMovePower(move);

    
    // going too need to balance this eventually
    return (power * committer->getAtkStat() / reciever->getDefStat()) * mod;
}

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


void BattleEngine::LoadOpponent(OpponentSeed* seed) {
    this->opponent->load(seed);


}

void BattleEngine::startEncounter() {
}

//this function is terrible
void BattleEngine::turnTick() {
    this->getInput();
    this->opponentInput();
    int8_t order = this->playerAction.priority != this->opponentAction.priority;
    if(order > 0){
        //player first
        this->commitAction(&this->playerAction, this->playerCur, this->opponentCur);
        this->commitAction(&this->opponentAction, this->opponentCur, this->playerCur);
    } else if (order < 0) {
        //opponent first
        this->commitAction(&this->opponentAction, this->opponentCur, this->playerCur);
        this->commitAction(&this->playerAction, this->playerCur, this->opponentCur);
    } else {
        order = this->playerCur->getSpdStat() - this->opponentCur->getSpdStat();
        if(order > 0 || order == 0){
            this->commitAction(&this->playerAction, this->playerCur, this->opponentCur);
            this->commitAction(&this->opponentAction, this->opponentCur, this->playerCur);

        } else if (order < 0) {
            //opponent first
            this->commitAction(&this->opponentAction, this->opponentCur, this->playerCur);
            this->commitAction(&this->playerAction, this->playerCur, this->opponentCur);
        }
    }
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
    // arduboy input from menu
    this->menu->actionInput(&this->playerAction);
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

void BattleEngine::LoadPlayer(Creature* playerParty[3]) {
    
    for(uint8_t i = 0; i < 3; i++) {
        this->playerParty[i] = playerParty[i];
        this->playerHealths[i] = this->playerParty[i]->getHpStat();
    }
}

void BattleEngine::commitAction(Action* action, Creature* commiter, Creature* reciever) {
    switch (action->actionType) {
    case ActionType_t::ATTACK: {
        uint16_t damage = calculateDamage(action, commiter, reciever);
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
        this->playerHealths[this->playerIndex] -= damage;
    } else {
        this->opponentHealths[this->opponentIndex] -= damage;

    }

}