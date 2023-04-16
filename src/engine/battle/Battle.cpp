#include "Battle.hpp"
#include "creature/Creature.hpp"
#include "opponent/Opponent.hpp"
#include "player/Player.hpp"
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
void BattleEngine::encounter(Player* player, OpponentSeed* seed) {
    this->startEncounter(player, seed);
    while(!this->checkLoss() || !this->checkWin()) {
        #ifdef CLI
            this->printEncounter();
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

    #ifdef CLI
        std::cout << "end encounter /n";
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
            std::cout << "PLAYER: cur: " << (unsigned)this->playerIndex << " lvl: " << (unsigned)this->playerCur->level << "\nhp:";
            for(int i = 0; i < 3; i++ ){ std::cout << " " << (unsigned)this->playerHealths[i];}
            std::cout << std::endl;
            std::cout << "Opponent: cur:" << (unsigned)this->opponentIndex << " lvl: " << (unsigned)this->opponentCur->level << "\nhp:";
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
        this->playerHealths[this->playerIndex] -= damage;
    } else {
        this->opponentHealths[this->opponentIndex] -= damage;

    }

}