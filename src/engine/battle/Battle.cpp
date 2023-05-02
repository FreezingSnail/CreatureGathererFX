#include "Arduboy2.h"
#include "Battle.hpp"
#include "../game/Menu.hpp"
#include "../../creature/Creature.hpp"
#include "../../opponent/Opponent.hpp"
#include "../../player/Player.hpp"
#include "../../lib/TypeTable.hpp"
#include "../../lib/Move.hpp"
#include "../../data/opponentsData.h"
#include "../../sprites/creatureSprites.h"



//#define DEBUG

uint16_t BattleEngine::calculateDamage(Action* action, Creature* committer, Creature* reciever) {
    //need to do something here with atk def stats
    uint8_t move = getMove(committer->moves[action->actionIndex]);
    float mod = getMatchupModifier(getMoveType(move), uint8_t(reciever->type))/2;
    uint8_t power = getMovePower(move);
    bool bonus = committer->moveTypeBonus(move);

    #ifdef DEBUG
        while(!this->arduboy->justPressed(B_BUTTON)){
            this->arduboy->pollButtons();
            this->arduboy->clear();
            this->arduboy->print(F("p:  atk:  def:\n"));
            this->arduboy->print((unsigned)power);  this->arduboy->print(F(" "));
            this->arduboy->print((unsigned)committer->getAtkStat());this->arduboy->print(F(" "));
            this->arduboy->print((unsigned)reciever->getDefStat());this->arduboy->print(F(" "));
            this->arduboy->print(F("\n"));
            this->arduboy->print(F("move Type: rtype: mod: \n"));
            this->arduboy->print((unsigned)getMoveType(move)); this->arduboy->print(F(" ")); 
            this->arduboy->print((unsigned)reciever->type); this->arduboy->print(F(" "));
            this->arduboy->print((mod));this->arduboy->print(F(" "));
            this->arduboy->display();
        }
    #endif

    uint16_t damage = ((power * committer->getStatAtLevel(committer->getAtkStat()) / reciever->getStatAtLevel(reciever->getDefStat())) * mod);
    if ( damage == 0 ){
        if (mod == 0) {
            return 0;
        }
        return 1;
    }

    if ( bonus ) {
        return damage * 2;
    }

    // going too need to balance this eventually
    return damage;
}

BattleEngine::BattleEngine(Arduboy2* arduboy, Menu* menu) {
    this->arduboy = arduboy;
    this->menu = menu;
}

// battle loop

//Need to change something here for the flow of the game
void BattleEngine::encounter() {

    if (this->checkLoss()) {
        this->arduboy->print(F("win\n"));
        this->arduboy->display();
        if(this->arduboy->justPressed(A_BUTTON)){
            return;
        }
    }

    if(this->checkWin()) {
        this->arduboy->print(F("loose\n"));
        this->arduboy->display();
        return;
    }

    this->turnTick();
    this->menu->printMenu();
    this->drawScene();
    #ifdef DEBUG
        //this->printEncounter();
        // this->playerCur->printMoves();
    #endif

    

   
}


void BattleEngine::loadOpponent(uint8_t optID) {
    OpponentSeed_t seed = OpponentSeed_t{0,0,1};

    memcpy_P( &seed, &opts[optID], sizeof( OpponentSeed_t));

    this->opponent.load(&seed);
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
     this->playerHealths[0] = 10;
    this->playerHealths[0] = this->playerParty[0]->getHpStat();
    this->playerHealths[1] = this->playerParty[1]->getHpStat();
    this->playerHealths[2] = this->playerParty[2]->getHpStat();
    
    this->playerIndex = 0;
    this->playerCur = this->playerParty[0];
}

void BattleEngine::startEncounter(Player* player, uint8_t optID) {
    this->arduboy->print("starting encounter\n");
    this->loadOpponent(optID);
    this->loadPlayer(player);
    this->menu->setState(State_t::BATTLE);
    //this->encounter();
}

//this function is terrible
void BattleEngine::turnTick() {
    if(!this->getInput()){
        return;
    }
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
    this->commitAction(&this->playerAction, this->playerCur, this->opponentCur);
    this->checkOpponentFaint();
    this->commitAction(&this->opponentAction, this->opponentCur, this->playerCur);
    this->checkPlayerFaint();
}

void BattleEngine::opponentActionFirst() {
    this->commitAction(&this->opponentAction, this->opponentCur, this->playerCur);
    this->checkPlayerFaint();
    this->commitAction(&this->playerAction, this->playerCur, this->opponentCur);
    this->checkOpponentFaint();

}

void BattleEngine::checkPlayerFaint() {
    if (this->playerHealths[this->playerIndex] <= 0 ){
        this->playerIndex++;
        this->playerCur = this->playerParty[this->playerIndex];
        //this->awakeMons &= ~(1 << this->playerIndex);
    }
}

void  BattleEngine::checkOpponentFaint() {
    if (this->opponentHealths[this->opponentIndex] <= 0 ){
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
    this->arduboy->print(F("\nend encounter \n"));
}

bool BattleEngine::getInput() {
    // arduboy input from menu
    return this->menu->actionInput(&this->playerAction);
}

void BattleEngine::opponentInput() {
    // ai to select best move

    //For now just do the first slot attack
    this->opponentAction.setActionType(ActionType::ATTACK);
    this->opponentAction.actionIndex = 1;
}

void BattleEngine::commitAction(Action* action, Creature* commiter, Creature* reciever) {
    switch (action->actionType) {
    case ActionType::ATTACK: {
        uint16_t damage = calculateDamage(action, commiter, reciever);
        applyDamage(damage, reciever);
        break;
    }
    case ActionType::ITEM:
        break;
    case ActionType::CHNGE:
        break;
    case ActionType::ESCAPE:
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

void BattleEngine::drawScene() {
    this->arduboy->drawRect(0,32, 128, 32);
    this->drawPlayer();
    this->drawOpponent();
}

void BattleEngine::drawOpponent() {
    //would be nice to flip this sprite
    uint8_t spriteIndex = this->opponentCur->getID();
    Sprites::drawSelfMasked(0, 0, creatureSprites, spriteIndex);
    this->drawOpponentHP();
}

void BattleEngine::drawPlayer() {
    uint8_t spriteIndex = this->playerCur->getID();
    Sprites::drawSelfMasked(96, 0, creatureSprites, spriteIndex);
    this->drawPlayerHP();
}

void BattleEngine::drawPlayerHP() {
    this->arduboy->setCursor(2,35);
    this->arduboy->print(F("HP: "));
    this->arduboy->print((unsigned)this->playerHealths[this->playerIndex]);
    this->arduboy->print(F("/"));
    this->arduboy->print((unsigned)this->playerCur->getHpStat());
    this->arduboy->setCursor(32,2);
    this->arduboy->print(F("lv:"));
    this->arduboy->print(this->playerCur->level);
}

void BattleEngine::drawOpponentHP() {
    this->arduboy->setCursor(70,35);
    this->arduboy->print(F("HP: "));
    this->arduboy->print((unsigned)this->opponentHealths[this->opponentIndex]);
    this->arduboy->print(F("/"));
    this->arduboy->print((unsigned)this->opponentCur->getHpStat());
    this->arduboy->setCursor(64,2);
    this->arduboy->print(F("lv:"));
    this->arduboy->print(this->opponentCur->level);
}


#ifdef DEBUG

        void BattleEngine::printEncounter() {
            while(!this->arduboy->justPressed(B_BUTTON)){
                this->arduboy->pollButtons();
                this->arduboy->clear();
                this->arduboy->setCursor(0,0);
                this->arduboy->print(F("P: cur: ")); this->arduboy->print((unsigned)this->playerIndex); 
                this->arduboy->print(F(" lvl: ")); this->arduboy->print((unsigned)this->playerCur->level);
                this->arduboy->print(F("\nhp:"));
                for(int i = 0; i < 3; i++ ){ this->arduboy->print(F(" ")); this->arduboy->print((unsigned)this->playerHealths[i]);}
                this->arduboy->print(F("\nO: cur: ")); this->arduboy->print((unsigned)this->opponentIndex); 
                this->arduboy->print(F(" lvl: ")); this->arduboy->print((unsigned)this->opponentCur->level);
                this->arduboy->print(F("\nhp:"));
                for(int i = 0; i < 3; i++ ){ this->arduboy->print(F(" ")); this->arduboy->print((unsigned)this->opponentHealths[i]);}
                this->arduboy->display();
            }
        }

#endif